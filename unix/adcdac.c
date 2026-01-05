#include "adcdac.h"
#include <string.h> // memset
#include <stdlib.h> // rand
#include "os/globals.h"
#include "os/taskscheduler.h"   // task_rc
#include "os/hal_plat.h"   // dac_sample_stream_callback // This should live here btw..


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


