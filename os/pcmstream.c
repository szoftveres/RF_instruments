#include "pcmstream.h"
#include "globals.h"
#include "hal_plat.h" // malloc, DAC, sampler

#include <string.h>


int wav_read_header (fs_broker_t* fs, int fd, int* samplerate, int* channels, int* bytespersample, int* samples) {
	riff_header_t riff;

	if (read_f(fs, fd, (char*)&riff, sizeof(riff_header_t)) < (int)sizeof(riff_header_t)) {
		console_printf("read fail");
		return 0;
	}

	if (memcmp(riff.riff_header, "RIFF", 4)) {
		console_printf("RIFF header");
		return 0;
	}
	if (memcmp(riff.wave.wave_header, "WAVE", 4)) {
		console_printf("WAVE header");
		return 0;
	}
	if (memcmp(riff.wave.fmt_header, "fmt ", 4)) {
		console_printf("fmt header");
		return 0;
	}
	if (riff.wave.fmt_chunk_size != 16) {
		console_printf("chunk size:%i", riff.wave.fmt_chunk_size);
		return 0;
	}
	if (riff.wave.audio_format != 1) {
		console_printf("audio format:%i", riff.wave.audio_format);
		return 0;
	}

	*samplerate = riff.wave.sample_rate;
	*channels = riff.wave.num_channels;
	*bytespersample = riff.wave.sample_alignment;
	*samples = riff.wave.data_bytes / riff.wave.sample_alignment;

	return 1;
}


int wav_write_header (fs_broker_t* fs, int fd, int samplerate, int channels, int bytespersample, int samples) {
	riff_header_t riff;

	memcpy(riff.riff_header, "RIFF", 4);
	riff.wav_size = (bytespersample * samples) + sizeof(wave_header_t);

	memcpy(riff.wave.wave_header, "WAVE", 4);

	memcpy(riff.wave.fmt_header, "fmt ", 4);
	riff.wave.fmt_chunk_size = 16;
	riff.wave.audio_format = 1;
	riff.wave.num_channels = channels;
	riff.wave.sample_rate = samplerate;
	riff.wave.byte_rate = bytespersample * samplerate;
	riff.wave.sample_alignment = bytespersample;
	riff.wave.bit_depth = (bytespersample / channels) * 8;

	memcpy(riff.wave.data_header, "data", 4);
	riff.wave.data_bytes = bytespersample * samples;

	if (write_f(fs, fd, (char*)&riff, sizeof(riff_header_t)) < 0) {
		console_printf("write fail");
		return 0;
	}

	return 1;
}

/* ========================*/
/* PCM audio WAV file sink */


typedef struct wavsink_context_s {
	uint16_t samplerate;
	fs_broker_t *fs;
	int fd;
	int samples;
	fifo_t* in_stream;
} wavsink_context_t;


task_rc_t wavsink_task (void* context) {
	uint16_t sample;
	wavsink_context_t *c = (wavsink_context_t*)context;

	if (!c->in_stream->writers) {
		return TASK_RC_END;
	}

	if (fifo_pop(c->in_stream, &sample)) {
		int16_t fmt_sample = (int16_t)(((int)sample) - 32768);
		if (!c->samplerate) {
			c->samplerate = sample;
			return TASK_RC_YIELD;
		}
		write_f(c->fs, c->fd, (char*)&fmt_sample, sizeof(int16_t));
		c->samples += 1;
	}

	return TASK_RC_YIELD;
}


void wavsink_celanup (void* context) {
	wavsink_context_t *c = (wavsink_context_t*)context;
	c->in_stream->readers--;
	rewind_f(c->fs, c->fd);
	wav_write_header(c->fs, c->fd, (int)c->samplerate, 1, 2, c->samples);
	close_f(c->fs, c->fd);
	t_free(context);
}


int wavsink_setup (fifo_t* in_stream, int fd) {
	if (!in_stream) {
		close_f(fs, fd);
		return 0;
	}
	wavsink_context_t* context = (wavsink_context_t*)t_malloc(sizeof(wavsink_context_t));
	context->in_stream = in_stream;
	context->in_stream->readers++;
	context->samplerate = 0;
	context->samples = 0;
	context->fs = fs;
	context->fd = fd;

	wav_write_header(context->fs, context->fd, (int)context->samplerate, 1, 2, context->samples);

	scheduler_install_task(scheduler, wavsink_task, wavsink_celanup, context);

	return 1;
}


/* ==========================*/
/* PCM audio DAC output sink */

typedef struct pcmsnk_context_s {

	int fs;
	int pending_sample;
	uint16_t pcmsample;
	fifo_t* in_stream;
	fifo_t* out_stream;

	task_rc_t (*consumer_fn) (void*, uint16_t);
	task_rc_t (*producer_fn) (void*, uint16_t*);
	void (*cleanup) (void*);
	void* function_context;
} pcmsnk_context_t;



/* ============================= */
/* Generic decimating FIR filter */

typedef struct firstream_context_s {
	int stuck_sample;
	uint16_t sample;

	int *buf;
	int bp;

	int *tap;
	int taps;
	int norm; // sum of taps, for normalization

	int dec; // decimation factor

	uint16_t samplerate;
	fifo_t* in_stream;
	fifo_t* out_stream;
} firstream_context_t;



task_rc_t firstream_task (void* context) {
	firstream_context_t *c = (firstream_context_t*)context;

	if (c->stuck_sample) {
		if (!c->out_stream->readers) {
			return TASK_RC_END;
		}
		if (!fifo_push(c->out_stream, &(c->sample))) {
			return TASK_RC_YIELD;
		}
		c->stuck_sample = 0;
	}

	if (!c->in_stream->writers) {
		if (!fifo_isempty(c->out_stream)) {
			return TASK_RC_YIELD; // Wait for all the samples to go out
		}
		return TASK_RC_END;
	}

	if (fifo_pop(c->in_stream, &(c->sample))) {
		if (!c->samplerate) {
			c->samplerate = c->sample;
			c->sample /= c->dec;
			c->bp = 0;
			c->stuck_sample = 1; // Communicating the output sample rate
			return TASK_RC_AGAIN;
		}

		int sample = ((int)c->sample) - 32768; // back to signed int

		c->buf[c->bp++] = sample;
		if (c->bp < c->taps) {
			return TASK_RC_AGAIN;
		}
		sample = fir_work(c->buf, c->tap, c->taps, c->dec) / c->norm;
		c->bp -= c->dec;

		c->sample = (uint16_t)(sample + 32768); // back to unsigned int

		if (!c->out_stream->readers) {
			return TASK_RC_END;
		}
		if (fifo_push(c->out_stream, &(c->sample))) {
			return TASK_RC_AGAIN;
		}
		c->stuck_sample = 1;
	}

	return TASK_RC_YIELD;
}


void firstream_celanup (void* context) {
	firstream_context_t *c = (firstream_context_t*)context;
	c->in_stream->readers--;
	c->out_stream->writers--;
	t_free(c->buf);
	t_free(c->tap);
	t_free(c);
}


int decfir_setup (fifo_t* in_stream, fifo_t* out_stream, int n, int bf) {
	int nbf;

	if (!in_stream || !out_stream) {
		return 0;
	}
	firstream_context_t* context = (firstream_context_t*)t_malloc(sizeof(firstream_context_t));
	context->stuck_sample = 0;
	context->in_stream = in_stream;
	context->in_stream->readers++;
	context->out_stream = out_stream;
	context->out_stream->writers++;
	context->samplerate = 0;



	context->dec = n;
	nbf = n * bf;
	context->taps = (2*nbf)-1;
	console_printf("df: taps=%i", context->taps);
	context->tap = (int*)t_malloc(context->taps * sizeof(int));
	int centertap = nbf-1;
	for (int i = 0; i != nbf; i++) {
		context->tap[centertap+i] = sinc_func(i, n*2);
		context->tap[centertap-i] = sinc_func(i, n*2);
		//console_printf("tap %i", context->tap[centertap+i]);
	}
	context->norm = fir_normf(context->tap, context->taps);
	context->buf = (int*)t_malloc(context->taps * sizeof(int));
	memset(context->buf, 0x00, context->taps * sizeof(int));

	scheduler_install_task(scheduler, firstream_task, firstream_celanup, context);

	return 1;
}


/* ====================*/
/* PCM audio NULL sink */


typedef struct nullsink_context_s {
	int samples;
	uint16_t samplerate;
	fifo_t* in_stream;
	uint16_t min;
	uint16_t max;
} nullsink_context_t;


task_rc_t nullsink_task (void* context) {
	uint16_t sample;
	nullsink_context_t *c = (nullsink_context_t*)context;

	if (!c->in_stream->writers) {
		return TASK_RC_END;
	}

	if (fifo_pop(c->in_stream, &sample)) {
		if (!c->samplerate) {
			c->samplerate = sample;
			return TASK_RC_YIELD;
		}
		c->samples += 1;
		if (sample < c->min) {
			c->min = sample;
		}
		if (sample > c->max) {
			c->max = sample;
		}
	}

	return TASK_RC_YIELD;
}


void nullsink_celanup (void* context) {
	nullsink_context_t *c = (nullsink_context_t*)context;
	c->in_stream->readers--;
	console_printf("fs:%i samples:%i, min:%i, max:%i", c->samplerate, c->samples, c->min, c->max);
	t_free(context);
}


int nullsink_setup (fifo_t* in_stream) {
	if (!in_stream) {
		return 0;
	}
	nullsink_context_t* context = (nullsink_context_t*)t_malloc(sizeof(nullsink_context_t));
	context->in_stream = in_stream;
	context->in_stream->readers++;
	context->samplerate = 0;
	context->samples = 0;
	context->min = 65535;
	context->max = 0;

	scheduler_install_task(scheduler, nullsink_task, nullsink_celanup, context);

	return 1;
}


/* ====================*/
/*    RXMODEM sink     */


// (Barker 4) x -(Barker 4)
#define PKT_PREAMBLE1 (0xB4)
// (Barker 5) x -(Barker 3)
#define PKT_PREAMBLE2 (0xE9)


#define PACKET_PAYLOAD_MAX (16)

typedef union __attribute__((packed)) modem_packet_u {
	uint8_t byte[0];
	struct __attribute__((packed)) {
		uint8_t preamble1;
		uint8_t preamble2;
		uint8_t len;
		//uint8_t ctrl;
        uint8_t crc;
		char payload[PACKET_PAYLOAD_MAX];
	};
} modem_packet_t;


uint8_t crc8 (uint8_t byte[], int len) {
    uint8_t crc = 0;

    for (int n = 0; n != len; n++) {
        crc ^= byte[n];

        for (int i = 0; i != 8; i++) {
            crc <<= 1;
            if (crc & 0x80) {
                crc ^= 0xE7;  // generator polynomial
            }
        }
    }
    return crc;
}

#define PACKET_TOTAL_LEN(l) ((int)(l) + ((int)sizeof(modem_packet_t) - PACKET_PAYLOAD_MAX))

int packetize (modem_packet_t* p, void *data, int len) {
    if (len > PACKET_PAYLOAD_MAX) {
        return 0;
    }
    p->len = (uint8_t)len;
    p->crc = 0;
    p->preamble1 = PKT_PREAMBLE1;
    p->preamble2 = PKT_PREAMBLE2;
    if (len) {
        memcpy(p->payload, data, len);
    }
    p->crc = crc8(p->byte, PACKET_TOTAL_LEN(p->len));
    return len;
}

int depacketize (modem_packet_t* p, void **data) {
    uint8_t crc = 0;
    if (p->len > PACKET_PAYLOAD_MAX) {
        return -1;
    }
    if (data) {
        *data = p->payload;
    }
    crc = p->crc;
    p->crc = 0;
    if (crc8(p->byte, PACKET_TOTAL_LEN(p->len)) != crc) {
        return -1;
    }
    return p->len;
}


#define MODEM_BPSK_BAUDRATE (400)
#define MODEM_RX_OVERSAMPLE_RATE (5)

typedef struct modem_context_s {
	int fft_len;
	int fc;
	dds_t* mixer;
	fifo_t* in_stream;
	fifo_t* out_stream;
	int* i;
	int* q;
	int wp;
	int sync;

	int cm;
	int cmi;

	modem_packet_t packet;
	int pp;

    uint8_t byte[MODEM_RX_OVERSAMPLE_RATE]; // XXX malloc
    int bitcounter;

	struct {  //         Decimating filter
		int *buf_i;
		int *buf_q;
		int bp;          // This tracks the incoming samples

		int *tap;
		int taps;
		int norm; // sum of taps, for normalization

		int target_fs;
		int dec; // decimation factor
	} ds;

    struct {
	    int* bitval;
    };

	int stuck_sample;
	uint16_t sample;
} modem_context_t;



task_rc_t bpsk_rxmodem_task (void* context) {
	uint16_t sample;
	modem_context_t *c = (modem_context_t*)context;
	int rss;

	if (!c->in_stream->writers) {
		return TASK_RC_END;
	}

	if (!fifo_pop(c->in_stream, &sample)) {
		return TASK_RC_YIELD;
	}
	if (!c->ds.dec) { // fs received

		c->mixer = dds_create(((int)sample), c->fc);

		c->ds.dec = ((int)sample) / c->ds.target_fs; //  dec =   fs / target fs
		int nbf = c->ds.dec * 2; // n * BF
		c->ds.taps = (2*nbf)-1;
		console_printf("rx: fs:%i fc:%i len:%i dec:%i taps:%i", ((int)sample), c->fc, c->fft_len, c->ds.dec, c->ds.taps);
		c->ds.tap = (int*)t_malloc(c->ds.taps * sizeof(int));
		int centertap = nbf-1;
		for (int i = 0; i != nbf; i++) {
			c->ds.tap[centertap+i] = sinc_func(i, c->ds.dec * 2);
			c->ds.tap[centertap-i] = sinc_func(i, c->ds.dec * 2);
		}
		c->ds.norm = fir_normf(c->ds.tap, c->ds.taps);
		c->ds.buf_i = (int*)t_malloc(c->ds.taps * sizeof(int));
		c->ds.buf_q = (int*)t_malloc(c->ds.taps * sizeof(int));

		c->wp = 0;
		c->ds.bp = 0;
		c->pp = -1;

		return TASK_RC_YIELD;
	}

    if (c->pp == PACKET_TOTAL_LEN(c->packet.len)) { // We're done
        int len = depacketize(&(c->packet), NULL);
        if (len >= 0) {
            console_printf("pkt:[%s]", c->packet.payload);
        }
        c->pp = -1;
        c->packet.len = PACKET_PAYLOAD_MAX;
        return TASK_RC_YIELD;
    }

	// Downconverting to baseband
	int i_a;
	int q_a;
	dds_next_sample(c->mixer, &i_a, &q_a);
	c->ds.buf_i[c->ds.bp] = (((int)sample) - 32768) * i_a / magnitude_const();
	c->ds.buf_q[c->ds.bp] = (((int)sample) - 32768) * q_a / magnitude_const();


	c->ds.bp++;
	if (c->ds.bp < c->ds.taps) {
		return TASK_RC_YIELD;
	}
	c->ds.bp -= c->ds.dec;  // due to memmove in fir_work

	// Downsampling (lowpass and decimate)
	c->wp = (c->wp + 1) % c->fft_len;              // move WP

    i_a = c->i[c->wp];              // Delayed conjugate
    q_a = -(c->q[c->wp]);           // Delayed conjugate
	c->i[c->wp] = fir_work(c->ds.buf_i, c->ds.tap, c->ds.taps, c->ds.dec) / c->ds.norm;
	c->q[c->wp] = fir_work(c->ds.buf_q, c->ds.tap, c->ds.taps, c->ds.dec) / c->ds.norm;

    // Polar discriminator
    c->bitval[c->wp] = ((i_a * c->i[c->wp]) - (q_a * c->q[c->wp])) / magnitude_const();
    // We only care about the real part after the polar discriminator

    int bit = 0;
    // matched filter
    for (int i = 0; i != c->fft_len; i++) {
        bit += c->bitval[i];
    }
    bit /= c->fft_len;
    rss = ((bit < 0) ? -bit : bit);
    c->byte[c->wp] <<= 1;
    c->byte[c->wp] |= (bit > 0 ? 0x00 : 0x01);

	switch (c->pp) {
		case -1: { // trying to sync to the preamble
			if (c->byte[c->wp] == PKT_PREAMBLE1) {
                //console_printf("sync start, rss:%i", rss);
				c->sync = 0;
				c->cm = rss;
				c->cmi = c->sync;
				c->pp += 1;  // Advance to the next state
                c->packet.byte[c->pp] = c->byte[c->wp];
			}
			return TASK_RC_YIELD;
		};
		case 0: { // Found a valid preamble, let's find the max
			c->sync += 1;
			if ((c->byte[c->wp] == PKT_PREAMBLE1) && (rss > c->cm)) { // Found a better peak
                //console_printf("sync        rss:%i", rss);
				c->cm = rss;
				c->cmi = c->sync;
			}
			if (c->sync > (c->fft_len - 2)) {
                //console_printf("sync end    sync:%i", c->sync);
				c->sync = (c->fft_len - c->sync) + c->cmi; // Samples to skip for best sync to the next symbol
                c->bitcounter = 0;
                //console_printf("sync end best sync:%i", c->sync);
				c->pp += 1; // Advance to the next state
			}
			return TASK_RC_YIELD;
		};
    }

	c->sync -= 1;
	if (c->sync) { // Just sinking samples
		return TASK_RC_YIELD;
	}

	c->sync = c->fft_len;
    c->bitcounter++;
    if (c->bitcounter < 8) {
        return TASK_RC_YIELD;
    }
    c->bitcounter = 0;

	c->packet.byte[c->pp] = c->byte[c->wp];
	c->cm += rss;
	c->sync = c->fft_len;
	if (((c->pp == 1) && (c->packet.byte[c->pp] != PKT_PREAMBLE2)) ||
        (c->packet.len > PACKET_PAYLOAD_MAX)) {
        //console_printf("lost it: :0x%02X, rss:%i", c->packet.byte[c->pp], rss);
		c->pp = -1; // We lost it
		c->wp = 0;
		return TASK_RC_YIELD;
	}
	c->pp += 1; // Advance to the next stage

	return TASK_RC_YIELD;
}

void modemcontext_celanup (void* context) {
	modem_context_t *c = (modem_context_t*)context;
    if (c->in_stream) {
	    c->in_stream->readers--;
    }
    if (c->out_stream) {
	    c->out_stream->writers--;
    }
	if (c->i) {
		t_free(c->i);
	}
	if (c->q) {
		t_free(c->q);
	}
	if (c->bitval) {
		t_free(c->bitval);
	}
	if (c->mixer) {
		dds_destroy(c->mixer);
	}
	if (c->ds.tap) {
		t_free(c->ds.tap);
	}
	if (c->ds.buf_i) {
		t_free(c->ds.buf_i);
	}
	if (c->ds.buf_q) {
		t_free(c->ds.buf_q);
	}
	t_free(context);
}


//  MODEM_BPSK_BAUDRATE
int rxmodem_setup (fifo_t* in_stream, int fc) {
	if (!in_stream) {
		return 0;
	}
	modem_context_t* context = (modem_context_t*)t_malloc(sizeof(modem_context_t));
	memset(context, 0x00, sizeof(modem_context_t));
	context->in_stream = in_stream;
	context->in_stream->readers++;
	context->fc = fc; // main carrier
	context->mixer = NULL; // yet unknown, fs depends on the sample rate

	context->fft_len = MODEM_RX_OVERSAMPLE_RATE; // carrier pairs * Nyquist * Oversample rate

	context->i = (int*)t_malloc(context->fft_len * sizeof(int));
	context->q = (int*)t_malloc(context->fft_len * sizeof(int));
	context->bitval = (int*)t_malloc(context->fft_len * sizeof(int));

	context->ds.target_fs = context->fft_len * MODEM_BPSK_BAUDRATE;

	context->ds.dec = 0;
    context->packet.len = PACKET_PAYLOAD_MAX;

	scheduler_install_task(scheduler, bpsk_rxmodem_task, modemcontext_celanup, context);

	return 1;
}


///////////////////////////////////////////////////////////////////////////


task_rc_t bpsk_txmodem_task (void* context) {
    modem_context_t *c = (modem_context_t*)context;

    if (c->stuck_sample) {
        if (!c->out_stream->readers) {
            return TASK_RC_END;
        }
        if (!fifo_push(c->out_stream, &(c->sample))) {
            return TASK_RC_YIELD;
        }
        c->stuck_sample = 0;
    }

    if (c->pp > PACKET_TOTAL_LEN(c->packet.len) + 1) { // We're done with the packet
        if (c->out_stream->readers && !fifo_isempty(c->out_stream)) {
            return TASK_RC_YIELD; // Wait for all the samples to go out
        }
        return TASK_RC_END;
    }

    if (c->wp) { // Upconvert c->i[c->ds.bp] c->q[c->ds.bp] and send out
        c->wp -= 1;

        int i_mix;
        int q_mix;
        dds_next_sample(c->mixer, &i_mix, &q_mix);
        c->sample = (((c->i[c->ds.bp] * i_mix) + (c->q[c->ds.bp] * q_mix)) / magnitude_const()) + 32768;

        if (!c->out_stream->readers) {
            return TASK_RC_END;
        }
        if (fifo_push(c->out_stream, &(c->sample))) {
            return TASK_RC_AGAIN;
        }
        c->stuck_sample = 1;
        return TASK_RC_YIELD;
    }
    c->wp = c->ds.dec;


    c->ds.bp += 1;
    if (c->ds.bp < c->fft_len) { // Go to the next baseband (oversampled) sample
        return TASK_RC_AGAIN;
    }
    c->ds.bp = 0;


    uint8_t nextbit = c->byte[c->ds.bp] & 0x80;
    c->byte[c->ds.bp] <<= 1;
    if (nextbit) { // differential encoding
        c->i[c->ds.bp] *= -1;
    }

    c->bitcounter++;
    if (c->bitcounter < 8) {
        return TASK_RC_YIELD;
    }
    c->bitcounter = 0;

    // Next byte
    c->pp += 1;

    if (c->pp < 0) {
        c->byte[c->ds.bp] = 0;
        c->i[c->ds.bp] = 16384;
        c->q[c->ds.bp] = 0;
    } else if (c->pp < PACKET_TOTAL_LEN(c->packet.len)) {
        c->byte[c->ds.bp] = c->packet.byte[c->pp];
        //console_printf("  -->  Tx encoding symbol %i", c->pp);
    } else {
        c->byte[c->ds.bp] = 0;
    }

    return TASK_RC_AGAIN;
}


int txmodem_setup (fifo_t* out_stream, int fs, int fc, char* msg) {
    if (!out_stream) {
        return 0;
    }
    modem_context_t* context = (modem_context_t*)t_malloc(sizeof(modem_context_t));
    memset(context, 0x00, sizeof(modem_context_t));
    context->out_stream = out_stream;
    context->out_stream->writers++;
    context->fc = fc; // main carrier
    context->mixer = dds_create(fs, context->fc);
    context->fft_len = 1; // carrier pairs * Nyquist * Oversample rate
    context->i = (int*)t_malloc(context->fft_len * sizeof(int));
    memset(context->i, 0x00, context->fft_len * sizeof(int));
    context->q = (int*)t_malloc(context->fft_len * sizeof(int));
    memset(context->q, 0x00, context->fft_len * sizeof(int));

    context->ds.target_fs = context->fft_len * MODEM_BPSK_BAUDRATE;
    context->ds.dec = fs / context->ds.target_fs;

    packetize(&(context->packet), msg, strlen(msg)+1);

    context->stuck_sample = 0;

    context->pp = -2;   // XXX should be -2
    context->bitcounter = 7;
    context->wp = 0;
    context->ds.bp = context->fft_len;

    console_printf("tx: fs:%i fc:%i len:%i", fs, context->fc, context->fft_len);

    uint16_t u16samplerate = (uint16_t)fs;
    fifo_push(context->out_stream, &u16samplerate);


    scheduler_install_task(scheduler, bpsk_txmodem_task, modemcontext_celanup, context);

    return 1;
}


/* ========================== */
/* Generic PCM audio source */

typedef struct pcmsrc_context_s {

	int fs_in;
	int fs_out;
	int pending_sample_out;
	int pending_sample_in;
	uint16_t sample_in;
	uint16_t sample_out;
	fifo_t* in_stream;
	fifo_t* out_stream;

	task_rc_t (*consumer_fn) (void*, uint16_t);
	task_rc_t (*producer_fn) (void*, uint16_t*);
	void (*cleanup) (void*);
	void* function_context;
} pcmsrc_context_t;



task_rc_t pcmsrc_task_flush_out (pcmsrc_context_t* c) {
	if (!c->out_stream) {
		return TASK_RC_END;
	}
	if (!c->out_stream->readers) {
		return TASK_RC_END;
	}
	if (!fifo_isempty(c->out_stream)) {
		return TASK_RC_YIELD; // Wait for all the samples to go out
	}
	return TASK_RC_END;
}


task_rc_t pcmsrc_task (void* context) {
	task_rc_t worker_rc;
	pcmsrc_context_t *c = (pcmsrc_context_t*)context;


	while (c->in_stream && c->consumer_fn) {
		if (c->pending_sample_in) {
			if (!c->fs_in) {
				c->fs_in = (int)c->sample_in;
				c->pending_sample_in = 0;
				continue; // The first incoming data is the sample rate
			}
			worker_rc = c->consumer_fn(c->function_context, c->sample_in);
			if (worker_rc == TASK_RC_END) {
				return pcmsrc_task_flush_out(c); // Done, flush the output and exit
			}
			if (worker_rc == TASK_RC_YIELD) {
				break;  // Could not consume/process the sample, let's try to push out
			}
			c->pending_sample_in = 0;
		}
		if (!fifo_pop(c->in_stream, &(c->sample_in))) {
			break;  // Could not pull in new sample, let's try to push out
		}
		c->pending_sample_in = 1;
	}

	while (c->out_stream && c->producer_fn) {
		if (c->pending_sample_out) {
			if (!c->out_stream->readers) {
				break; // Reader detached, exit
			}
			if (!fifo_push(c->out_stream, &(c->sample_out))) {
				return TASK_RC_YIELD; // output full, maybe next time
			}
			c->pending_sample_out = 0; // sample went out successfully
		}
		worker_rc = c->producer_fn(c->function_context, &(c->sample_out));
		if (worker_rc == TASK_RC_END) {
			break; // Done, flush the output and exit
		}
		if (worker_rc == TASK_RC_YIELD) { // no new sample
			return TASK_RC_YIELD;
		}
		c->pending_sample_out = 1; // Send the new sample out at the next cycle
	}

	return pcmsrc_task_flush_out(c);
}


void pcmsrc_celanup (void* context) {
	pcmsrc_context_t *c = (pcmsrc_context_t*)context;
	if (c->in_stream) {
		c->in_stream->readers--;
	}
	if (c->out_stream) {
		c->out_stream->writers--;
	}
	if (c->function_context) {
		if (c->cleanup) {
			c->cleanup(c->function_context);
		}
		t_free(c->function_context);
	}
	t_free(context);
}


int pcmsrc_setup (fifo_t* out_stream, int fs, task_rc_t (*fn) (void*, uint16_t*), void (*cleanup) (void*), void* function_context) {
	if (!out_stream) {
		return 0;
	}

	pcmsrc_context_t* context = (pcmsrc_context_t*)t_malloc(sizeof(pcmsrc_context_t));
	if (!context) {
		// XXX error msg, cleanup
		return 0;
	}

	context->pending_sample_in = 0;
	context->pending_sample_out = 0;
	context->fs_out = fs;

	context->in_stream = NULL;
	context->consumer_fn = NULL;

	context->out_stream = out_stream;
	context->out_stream->writers++;
	context->producer_fn = fn;

	context->cleanup = cleanup;
	context->function_context = function_context;

	uint16_t u16samplerate = (uint16_t)fs;
	fifo_push(context->out_stream, &u16samplerate);

	scheduler_install_task(scheduler, pcmsrc_task, pcmsrc_celanup, context);

	return 1;
}


