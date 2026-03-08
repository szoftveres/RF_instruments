#include "rfport_rx.h"
#include "../os/dsp_maths.h"
#include "../os/hal_plat.h"
#include "../os/fifo.h"
#include <string.h> //memset
#include "stm32h7xx_hal.h"


extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;


#define RFPORTRX_FS (80000)


typedef struct rfport_rx_sample_s {
	uint16_t ref;
	uint16_t meas;
} rfport_rx_sample_t;


static fifo_t* rfport_rx_sample_stream;


void rfport_rx_daq_callback (void* ctxt) {
	rfport_rx_sample_t sample;

	sample.ref = hadc1.Instance->DR;
	sample.meas = hadc2.Instance->DR;

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

	rfport_rx_daq_on();

	memset(m, 0x00, sizeof(rfport_rx_t));

	mixer = dds_create(RFPORTRX_FS, fc);

	// The first sample is garbage (leftover), discard it
	fifo_pop_or_sleep(rfport_rx_sample_stream, &sample);

	for (int c = 0; c != samples; c += 1) {
		int i;
		int q;
		int mag = magnitude_const();

		//int window = raised_cos_window(c, cycles);

		dds_next_sample(mixer, &i, &q);

		fifo_pop_or_sleep(rfport_rx_sample_stream, &sample);

		m->ref_i += ((((int)sample.ref) * i) / mag);
		m->ref_q += ((((int)sample.ref) * q) / mag);
		m->meas_i += ((((int)sample.meas) * i) / mag);
		m->meas_q += ((((int)sample.meas) * q) / mag);
	}

	m->ref_i /= samples;
	m->ref_q /= samples;
	m->meas_i /= samples;
	m->meas_q /= samples;

	dds_destroy(mixer);

	rfport_rx_daq_off();
}

