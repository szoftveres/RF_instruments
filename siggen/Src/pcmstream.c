#include "pcmstream.h"
#include "instances.h"
#include "hal_plat.h" // malloc, DAC, sampler

#include <string.h>

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


int fir_work (int buf[], int tap[], int taps, int dec) {
	int result = 0;
	for (int t = 0; t != taps; t += 1) {
		result += buf[t] * tap[t];
	}
	memmove(buf, &(buf[dec]), (taps-dec)*sizeof(int));

	return result;
}


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

	context->bp = 0;

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
	context->norm = 0;
	for (int i = 0; i != context->taps; i++) {
		context->norm += context->tap[i];
	}
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


#define PKT_PREAMBLE1 (0xCA)
#define PKT_PREAMBLE2 (0xB2)


typedef union __attribute__((packed)) modem_packet_u {
	uint8_t byte[0];
	struct {
		uint8_t preamble1;
		uint8_t preamble2;
		uint8_t len;
		uint8_t chksum;
		char payload[16];
	};
} modem_packet_t;


typedef struct modem_context_s {
	int sps;
	int len;
	int fc;
	dds_t* mixer;
	fifo_t* in_stream;
	fifo_t* out_stream;
	int* i;
	int* q;
	int* wave;
	int wp;
	int sync;

	int cm;
	int cmi;

	modem_packet_t packet;
	int pp;

	int stuck_sample;
	uint16_t sample;
} modem_context_t;


task_rc_t rxmodem_task (void* context) {
	uint16_t sample;
	modem_context_t *c = (modem_context_t*)context;
	char byte;
	int pilot_i;
	int pilot_q;

	if (!c->in_stream->writers) {
		return TASK_RC_END;
	}

	if (fifo_pop(c->in_stream, &sample)) {
		if (!c->len) {
			c->len = ((int)sample) / c->sps;
			c->i = (int*)t_malloc(c->len * sizeof(int));
			c->q = (int*)t_malloc(c->len * sizeof(int));
			c->wave = (int*)t_malloc(c->len * sizeof(int));
			c->pp = -1;
			c->mixer = dds_create(((int)sample), c->fc);
			console_printf("rx: fs:%i fc:%i len:%i", sample, c->fc, c->len);
			return TASK_RC_YIELD;
		}

		switch (c->pp) {
			case -1: { // trying to sync to the preamble
				memmove(c->wave, &(c->wave[1]), (c->len - 1) * sizeof(int)); // Shift the samples forward
				c->wave[c->len - 1] = ((int)sample) - 32768;    // Put the new sample at the back of the stream
				dds_reset(c->mixer);
				cplx_downconvert(c->mixer, c->wave, c->i, c->q, c->len);
				byte = ofdm_cplx_decode_u8(c->i, c->q, &pilot_i, &pilot_q, c->len);
				if (byte == PKT_PREAMBLE1 && (pilot_i > 0)) {
					c->wp = 0;
					c->cm = pilot_i;
					c->cmi = c->wp;
					c->pp += 1;  // Advance to the next state
					return TASK_RC_YIELD;
				}
				return TASK_RC_YIELD;
			};
			case 0: { // Found a valid preamble, let's find the max
				c->wp += 1;
				memmove(c->wave, &(c->wave[1]), (c->len - 1) * sizeof(int)); // Shift the samples forward
				c->wave[c->len - 1] = ((int)sample) - 32768;    // Put the new sample at the back of the stream
				dds_reset(c->mixer);
				cplx_downconvert(c->mixer, c->wave, c->i, c->q, c->len);
				byte = ofdm_cplx_decode_u8(c->i, c->q, &pilot_i, &pilot_q, c->len);
				if ((byte == PKT_PREAMBLE1) && (pilot_i > c->cm)) { // Found a better peak
					c->cm = pilot_i;
					c->cmi = c->wp;
				}
				if (c->wp > (c->len - 2)) {
					c->wp = c->len - (c->wp - c->cmi); // Samples to skip for best sync to the next symbol
					c->pp += 1; // Advance to the next state
					dds_reset(c->mixer);
					for (int i = 0; i != c->len; i++) {
						dds_next_sample(c->mixer, &pilot_i, &pilot_q); // replaying the mixer from the start
					}
					return TASK_RC_YIELD;
				}
				return TASK_RC_YIELD;
			};
			default: {
				if (c->pp == sizeof(modem_packet_t)) { // We're done
					console_printf("pkt (len:0x%02x, pilot:%i)", c->packet.len, c->cm/c->pp);
					c->pp = -1;
					return TASK_RC_YIELD;
				}
				memmove(c->wave, &(c->wave[1]), (c->len - 1) * sizeof(int)); // Shift the samples forward
				c->wave[c->len - 1] = ((int)sample) - 32768;    // Put the new sample at the back of the stream
				c->wp -= 1;
				if (c->wp) { // Just sinking samples
					return TASK_RC_YIELD;
				}
				//dds_reset(c->mixer); // XXX (wrong) assumption for now is that every symbol comes with an aligned carrier
				cplx_downconvert(c->mixer, c->wave, c->i, c->q, c->len);
				c->packet.byte[c->pp] = ofdm_cplx_decode_u8(c->i, c->q, &pilot_i, &pilot_q, c->len);
				c->cm += pilot_i;
				c->wp = c->len;
				if ((c->pp == 1) && ((c->packet.byte[c->pp] != PKT_PREAMBLE2) || pilot_i < 0)) {
					c->pp = -1; // We lost it
					return TASK_RC_YIELD;
				}
				c->pp += 1; // Advance to the next stage
				return TASK_RC_YIELD;
			}
		}
	}

	return TASK_RC_YIELD;
}


void rxmodem_celanup (void* context) {
	modem_context_t *c = (modem_context_t*)context;
	c->in_stream->readers--;
	if (c->i) {
		t_free(c->i);
	}
	if (c->q) {
		t_free(c->q);
	}
	if (c->wave) {
		t_free(c->wave);
	}
	if (c->mixer) {
		dds_destroy(c->mixer);
	}
	t_free(context);
}

int rxmodem_setup (fifo_t* in_stream) {
	if (!in_stream) {
		return 0;
	}
	modem_context_t* context = (modem_context_t*)t_malloc(sizeof(modem_context_t));
	context->in_stream = in_stream;
	context->in_stream->readers++;
	context->sps = 125;
	context->len = 0; // yet unknown, depends on the sample rate
	context->fc = 750; // main carrier
	context->mixer = NULL; // yet unknown, fs depends on the sample rate

	context->i = NULL; // yet unknown, length depends on the sample rate
	context->q = NULL; // yet unknown, length depends on the sample rate
	context->wave = NULL; // yet unknown, length depends on the sample rate
	scheduler_install_task(scheduler, rxmodem_task, rxmodem_celanup, context);

	return 1;
}



///////////////////////////////////////////////////////////////////////////




task_rc_t txmodem_task (void* context) {
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

	if (c->wp >= c->len) {
		c->pp += 1;
		c->wp = 0;
	}

	if (c->pp > (int)sizeof(modem_packet_t)) {
		if (!fifo_isempty(c->out_stream)) {
			return TASK_RC_YIELD; // Wait for all the samples to go out
		}
		return TASK_RC_END;
	}

	if (!c->wp) {
		uint8_t byte = 0x00;
		if ((c->pp >= 0) && (c->pp < sizeof(modem_packet_t))) {
			byte = c->packet.byte[c->pp];
		}
		c->cm = ofdm_cplx_encode_u8(byte, 1, 0, c->i, c->q, c->len, 65536);
		//dds_reset(c->mixer);
		cplx_upconvert(c->mixer, c->i, c->q, c->wave, c->len);
	}

	c->sample = (uint16_t)(c->wave[c->wp] + 32768);
	c->wp += 1;
	if (!c->out_stream->readers) {
		return TASK_RC_END;
	}
	if (fifo_push(c->out_stream, &(c->sample))) {
		return TASK_RC_AGAIN;
	}
	c->stuck_sample = 1;

	return TASK_RC_YIELD;
}


void txmodem_celanup (void* context) {
	modem_context_t *c = (modem_context_t*)context;
	c->out_stream->writers--;
	if (c->i) {
		t_free(c->i);
	}
	if (c->q) {
		t_free(c->q);
	}
	if (c->wave) {
		t_free(c->wave);
	}
	if (c->mixer) {
		dds_destroy(c->mixer);
	}
	t_free(context);
}


int txmodem_setup (fifo_t* out_stream, int fs) {
	if (!out_stream) {
		return 0;
	}
	modem_context_t* context = (modem_context_t*)t_malloc(sizeof(modem_context_t));
	context->sps = 125;
	context->len = fs / context->sps;
	context->fc = 750; // main carrier
	context->mixer = dds_create(fs, context->fc);
	context->pp = -1;
	context->wp = 0;

	context->i = (int*)t_malloc(context->len * sizeof(int));
	context->q = (int*)t_malloc(context->len * sizeof(int));
	context->wave = (int*)t_malloc(context->len * sizeof(int));

	context->packet.preamble1 = PKT_PREAMBLE1;
	context->packet.preamble2 = PKT_PREAMBLE2;
	context->packet.len = 0x42;
	context->packet.chksum = 0xBE;
	context->stuck_sample = 0;

	console_printf("tx: fs:%i fc:%i len:%i", fs, context->fc, context->len);

	context->out_stream = out_stream;
	context->out_stream->writers++;
	uint16_t u16samplerate = (uint16_t)fs;
	fifo_push(context->out_stream, &u16samplerate);

	scheduler_install_task(scheduler, txmodem_task, txmodem_celanup, context);

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


