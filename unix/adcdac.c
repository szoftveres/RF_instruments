#include "adcdac.h"
#include <string.h> // memset
#include <stdlib.h> // rand
#include <pulse/error.h> // 'apt-get install libpulse-dev'
#include <pulse/simple.h>
#include "os/globals.h"
#include "os/taskscheduler.h"   // task_rc
#include "os/hal_plat.h"   // dac_sample_stream_callback // This should live here btw..


#define UNIX_ADCDAC_SAMPLEBUF (32768)


static pa_simple *s_in = NULL;
static int16_t in_samplebuf[UNIX_ADCDAC_SAMPLEBUF];
static int in_rp = 0;


static pa_simple *s_out = NULL;
static int16_t out_samplebuf[UNIX_ADCDAC_SAMPLEBUF];
static int out_wp = 0;
static int out_fs;


int start_audio_in (int fs) {
    pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = fs,
        .channels = 1
    };
    in_rp = 0;
    s_in = pa_simple_new(NULL, "instr-os/unix", PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, NULL);
    if (!s_in) {
        return 0;
    }
    return 1;
}


void stop_audio_in (void) {
    if (s_in) {
        pa_simple_drain(s_in, NULL);
        pa_simple_flush(s_in, NULL);
        pa_simple_free(s_in);
    }
    in_rp = 0;
    s_in = NULL;
}


int start_audio_out (int fs) {
    pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = fs,
        .channels = 1
    };
    out_fs = fs;
    out_wp = 0;
    s_out = pa_simple_new(NULL, "instr-os/unix", PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, NULL);
    if (!s_out) {
        return 0;
    }

    // XXX  This is a hack, 0.5sec of noise, to wake up the sound card:
    for (int n = 0; n != UNIX_ADCDAC_SAMPLEBUF; n += 1) {
        for (int i = -1; i != fs / UNIX_ADCDAC_SAMPLEBUF / 2; i++) {
             int16_t sample_out = rand() % 32;
             play_int16_sample(&sample_out);
        }
    }

    return 1;
}


void stop_audio_out (void) {
    if (s_out) {
        // XXX  This is a hack, 0.1sec of graceful noise at the end
        for (int n = 0; n != UNIX_ADCDAC_SAMPLEBUF; n += 1) {
            for (int i = -1; i != out_fs / UNIX_ADCDAC_SAMPLEBUF / 2; i++) {
                int16_t sample_out = rand() % 32;
                play_int16_sample(&sample_out);
            }
        }
        if (out_wp) {
            pa_simple_write(s_out, out_samplebuf, out_wp * sizeof(int16_t), NULL);
        }
        pa_simple_drain(s_out, NULL);
        pa_simple_flush(s_out, NULL);
        pa_simple_free(s_out);
    }
    out_wp = 0;
    s_out = NULL;
}


int record_int16_sample (int16_t *s) {
    if (!s_in) {
        return 0;
    }
    if (!in_rp) {
        pa_simple_read(s_in, in_samplebuf, UNIX_ADCDAC_SAMPLEBUF * sizeof(int16_t), NULL);
    }
    *s = in_samplebuf[in_rp];
    in_rp = (in_rp + 1) % UNIX_ADCDAC_SAMPLEBUF;
    return 1;
}


int play_int16_sample (int16_t *s) {
    if (!s_out) {
        return 0;
    }
    out_samplebuf[out_wp] = *s;
    out_wp = (out_wp + 1) % UNIX_ADCDAC_SAMPLEBUF;
    if (!out_wp) {
        pa_simple_write(s_out, out_samplebuf, UNIX_ADCDAC_SAMPLEBUF * sizeof(int16_t), NULL);
    }
    return 1;
}


/* ==========================*/
/* PCM audio output sink */


typedef struct unix_adcdac_context_s {
	uint16_t samplerate;
	fifo_t* in_stream;
	fifo_t* out_stream;

    int16_t sample;
    int stuck_sample;
} unix_adcdac_context_t;


task_rc_t dacsink_task (void* context) {
    uint16_t sample;
	unix_adcdac_context_t *c = (unix_adcdac_context_t*)context;

	if (!c->in_stream->writers) {
		return TASK_RC_END;
	}

	if (fifo_pop(c->in_stream, &sample)) {
		if (!c->samplerate) {
            c->samplerate = sample;
            start_audio_out(c->samplerate);
			return TASK_RC_YIELD;
		}
        int16_t formatted_sample = (int16_t)(((int)sample) - 32768);
        play_int16_sample(&formatted_sample);
	}
	return TASK_RC_YIELD;
}


void adcdac_celanup (void* context) {
	unix_adcdac_context_t *c = (unix_adcdac_context_t*)context;
	if (c->in_stream) {
        c->in_stream->readers--;
        stop_audio_out();
    }
	if (c->out_stream) {
        c->out_stream->writers--;
        stop_audio_in();
    }
	t_free(context);
}


int dacsink_setup (fifo_t* in_stream) {
	if (!in_stream) {
		return 0;
	}
	unix_adcdac_context_t* context = (unix_adcdac_context_t*)t_malloc(sizeof(unix_adcdac_context_t));
	memset(context, 0x00, sizeof(unix_adcdac_context_t));
	context->in_stream = in_stream;
	context->in_stream->readers++;
	context->samplerate = 0;

	scheduler_install_task(scheduler, dacsink_task, adcdac_celanup, context);

	return 1;
}

/* =============================== */

task_rc_t adcsrc_task (void* context) {
    unix_adcdac_context_t *c = (unix_adcdac_context_t*)context;
    int16_t formatted_sample;

    if (c->stuck_sample) {
        if (!c->out_stream->readers) {
            return TASK_RC_END;
        }
        if (!fifo_push(c->out_stream, &(c->sample))) {
            return TASK_RC_YIELD;
        }
        c->stuck_sample = 0;
    }

    record_int16_sample(&formatted_sample);
    c->sample = (int16_t)(((int)formatted_sample) + 32768);

    if (!c->out_stream->readers) {
        return TASK_RC_END;
    }
    if (fifo_push(c->out_stream, &(c->sample))) {
        return TASK_RC_AGAIN;
    }
    c->stuck_sample = 1;
    return TASK_RC_YIELD;
}


int adcsrc_setup (fifo_t* out_stream, int fs) {
    if (!out_stream) {
        return 0;
    }
    unix_adcdac_context_t* context = (unix_adcdac_context_t*)t_malloc(sizeof(unix_adcdac_context_t));
    memset(context, 0x00, sizeof(unix_adcdac_context_t));

    context->out_stream = out_stream;
    context->out_stream->writers++;
    context->stuck_sample = 0;

    uint16_t u16samplerate = (uint16_t)fs;
    fifo_push(context->out_stream, &u16samplerate);

    start_audio_in(fs);

    scheduler_install_task(scheduler, adcsrc_task, adcdac_celanup, context);

    return 1;
}


