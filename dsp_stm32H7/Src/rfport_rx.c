#include "rfport_rx.h"
#include "../os/dsp_maths.h"
#include "../os/hal_plat.h"
#include "../os/fifo.h"
#include <string.h> //memset
#include <limits.h> //int max and int min
#include "stm32h7xx_hal.h"


extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;


#define RFPORTRX_FS (80000)


typedef struct rfport_rx_sample_s {
	int ref;
	int meas;
} rfport_rx_sample_t;


static fifo_t* rfport_rx_sample_stream;


void rfport_rx_daq_callback (void* ctxt) {
	rfport_rx_sample_t sample;

	sample.ref = ((int)hadc1.Instance->DR) - 32768;
	sample.meas = ((int)hadc2.Instance->DR) - 32768;

	HAL_ADC_Start(&hadc1);
	HAL_ADC_Start(&hadc2);
	fifo_push(rfport_rx_sample_stream, &sample);
}


void rfport_rx_daq_on (void) {
	rfport_rx_sample_stream = fifo_create(64, sizeof(rfport_rx_sample_t));

	set_sampler_frequency(RFPORTRX_FS);

	start_sampler(rfport_rx_daq_callback, NULL);
}


void rfport_rx_daq_off (void) {
	stop_sampler();

	fifo_destroy(rfport_rx_sample_stream);
}


void rfport_rx_meas (int fc, int samples, rfport_rx_t* m) {
	dds_t* mixer;
	rfport_rx_sample_t sample;
	int min = INT_MAX;
	int max = INT_MIN;

	rfport_rx_daq_on();

	memset(m, 0x00, sizeof(rfport_rx_t));

	mixer = dds_create(RFPORTRX_FS, fc);

	// The first sample is garbage (leftover), discard it
	fifo_pop_or_sleep(rfport_rx_sample_stream, &sample);
	fifo_pop_or_sleep(rfport_rx_sample_stream, &sample);

	for (int c = 0; c != samples; c += 1) {
		int i;
		int q;
		int mag = magnitude_const();

		//int window = raised_cos_window(c, samples);

		dds_next_sample(mixer, &i, &q);

		fifo_pop_or_sleep(rfport_rx_sample_stream, &sample);

		if (sample.ref < min) {min = sample.ref;}
		if (sample.ref > max) {max = sample.ref;}

		//sample.ref = sample.ref * window / mag;
		//sample.meas = sample.meas * window / mag;

		m->ref_i += ((sample.ref * i) / mag);
		m->ref_q += ((sample.ref * q) / mag);
		m->meas_i += ((sample.meas * i) / mag);
		m->meas_q += ((sample.meas * q) / mag);
	}

	m->ref_i /= samples;
	m->ref_q /= samples;
	m->meas_i /= samples;
	m->meas_q /= samples;
	m->ref_ampl = max - min;

	dds_destroy(mixer);

	rfport_rx_daq_off();
}

