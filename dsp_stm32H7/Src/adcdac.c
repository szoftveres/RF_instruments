#include "adcdac.h"
#include <stdint.h>
#include "../os/globals.h"
#include "../os/taskscheduler.h"   // task_rc
#include "../os/hal_plat.h"   // dac_sample_stream_callback // This should live here btw..

#include "stm32h7xx_hal.h"


extern DAC_HandleTypeDef hdac1;

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern ADC_HandleTypeDef hadc3;


void dac1_outv (uint32_t aval) {
	HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, aval);
	HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
}



void adc1_sample_stream_callback (void* ctxt) {
	uint16_t aval = HAL_ADC_GetValue(&hadc1);
	fifo_push((fifo_t*)ctxt, &aval);
	HAL_ADC_Start(&hadc1);
}

void dac_sample_stream_callback (void* ctxt) {
	uint16_t aval;
	if (fifo_pop((fifo_t*)ctxt, &aval)) {
		dac1_outv(aval & 0xFFF);
	}
}

int dac_max (void) {
	return 4096;
}

/* ==========================*/
/* PCM audio DAC output sink */

typedef struct dacsink_context_s {
	int stuck_sample;
	uint16_t sample;

	uint16_t samplerate;
	fifo_t* in_stream;
	fifo_t* dac_stream;
} dacsink_context_t;


task_rc_t dacsink_task (void* context) {
	dacsink_context_t *c = (dacsink_context_t*)context;

	if (c->stuck_sample) {
		if (!fifo_push(c->dac_stream, &(c->sample))) {
			return TASK_RC_YIELD;
		}
		c->stuck_sample = 0;
	}

	if (!c->in_stream->writers) {
		if (!fifo_isempty(c->dac_stream)) {
			return TASK_RC_YIELD; // Wait for all the samples to go out
		}
		return TASK_RC_END;
	}

	if (fifo_pop(c->in_stream, &(c->sample))) {
		if (!c->samplerate) {
			c->samplerate = c->sample;
			set_sampler_frequency((int)c->samplerate);
			start_sampler(dac_sample_stream_callback, c->dac_stream);
			return TASK_RC_YIELD;
		}
		c->sample /= 16; // 16 bit >> 12 bit

		if (fifo_push(c->dac_stream, &(c->sample))) {
			return TASK_RC_AGAIN;
		}
		c->stuck_sample = 1;
	}

	return TASK_RC_YIELD;
}


void dacsink_celanup (void* context) {
	dacsink_context_t *c = (dacsink_context_t*)context;
	c->in_stream->readers--;
	stop_sampler();
	fifo_destroy(c->dac_stream);
	t_free(context);
}


int dacsink_setup (fifo_t* in_stream) {
	if (!in_stream) {
		return 0;
	}
	dacsink_context_t* context = (dacsink_context_t*)t_malloc(sizeof(dacsink_context_t));
	context->stuck_sample = 0;
	context->in_stream = in_stream;
	context->in_stream->readers++;
	context->samplerate = 0;

	context->dac_stream = fifo_create(4096, sizeof(uint16_t));

	scheduler_install_task(scheduler, dacsink_task, dacsink_celanup, context);

	return 1;
}

/* ==========================*/
/* PCM audio ADC source */

typedef struct adcsrc_context_s {
	int stuck_sample;
	uint16_t sample;

	uint16_t samplerate;
	fifo_t* out_stream;
	fifo_t* adc_stream;
} adcsrc_context_t;


task_rc_t adcsrc_task (void* context) {
	adcsrc_context_t *c = (adcsrc_context_t*)context;

	if (!c->out_stream->readers) {
		return TASK_RC_END;
	}

	if (c->stuck_sample) {
		if (!fifo_push(c->out_stream, &(c->sample))) {
			return TASK_RC_YIELD;
		}
		c->stuck_sample = 0;
	}

	if (fifo_pop(c->adc_stream, &(c->sample))) {
		if (fifo_push(c->out_stream, &(c->sample))) {
			return TASK_RC_AGAIN;
		}
		c->stuck_sample = 1;
	}

	return TASK_RC_YIELD;
}


void adcsrc_celanup (void* context) {
	adcsrc_context_t *c = (adcsrc_context_t*)context;
	c->out_stream->writers--;
	stop_sampler();
	fifo_destroy(c->adc_stream);
	t_free(context);
}


int adcsrc_setup (fifo_t* out_stream, int fs) {
	if (!out_stream) {
		return 0;
	}
	adcsrc_context_t* context = (adcsrc_context_t*)t_malloc(sizeof(adcsrc_context_t));
	context->stuck_sample = 0;
	context->out_stream = out_stream;
	context->out_stream->writers++;
	context->samplerate = fs;

	fifo_push(context->out_stream, &(context->samplerate)); //sending FS

	context->adc_stream = fifo_create(4096, sizeof(uint16_t));

	scheduler_install_task(scheduler, adcsrc_task, adcsrc_celanup, context);
	set_sampler_frequency(fs);
	start_sampler(adc1_sample_stream_callback, context->adc_stream);

	fifo_pop_or_sleep(context->adc_stream, &(context->sample)); // Throw out the first sample

	return 1;
}

