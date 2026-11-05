#include "rfport_rx.h"
#include "../os/dsp_maths.h"
#include "../os/hal_plat.h"
#include <string.h> //memset
#include <limits.h> //int max and int min
#include "../os/globals.h" // stdio
#include "stm32h7xx_hal.h"


extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;


fifo_t* rfport_rx_sample_stream;


void rfport_rx_daq_callback (void* ctxt) {
	rfport_rx_sample_t sample;

	sample.ch1 = ((int)hadc1.Instance->DR) - 32768;
	sample.ch2 = ((int)hadc2.Instance->DR) - 32768;

	HAL_ADC_Start(&hadc1);

	fifo_push(rfport_rx_sample_stream, &sample);
}


void rfport_rx_daq_on (int fs, int bufdepth) {
	rfport_rx_sample_stream = fifo_create(bufdepth, sizeof(rfport_rx_sample_t));

	set_sampler_frequency(fs);
	HAL_ADC_Start(&hadc2);
	start_sampler(rfport_rx_daq_callback, NULL);
}


void rfport_rx_daq_off (void) {
	stop_sampler();
	HAL_ADC_Stop(&hadc1);
	HAL_ADC_Stop(&hadc2);
	fifo_destroy(rfport_rx_sample_stream);
}

extern DAC_HandleTypeDef hdac1;
void adctest (int fs, int ch) {
	int cycle = 0;
	int min = 65536 * 1024;
	int max = -min;
	rfport_rx_daq_on(fs, 1024);

	while (!pushbuttonstate()) {
		int s;
		rfport_rx_sample_t sample;
		fifo_pop_or_sleep(rfport_rx_sample_stream, &sample);
		s = (ch == 1) ? sample.ch1 : (ch == 2) ? sample.ch2 : 0;

		if (s < min) {min = s;}
		if (s > max) {max = s;}

		uint32_t aval = ((uint32_t)(s + 32768))/16;

		HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, aval);
		HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
		if (!cycle) {
			cycle = 44100;
			printf_f(STDOUT, "[%i] [%i]", min, max);
			min = 65536 * 1024;
			max = -min;
		}
		cycle -= 1;
	}
	rfport_rx_daq_off();
}


void rfport_rx_meas (int fs, int fc, int samples, rfport_rx_t* m, int window) {
	dds_t* mixer;
	rfport_rx_sample_t sample;
	int rmin = INT_MAX;
	int rmax = INT_MIN;
	int mmin = INT_MAX;
	int mmax = INT_MIN;

	rfport_rx_daq_on(fs, 64);

	memset(m, 0x00, sizeof(rfport_rx_t));

	mixer = dds_create(fs, fc, sinewave);

	// The first sample is garbage (leftover), discard it
	fifo_pop_or_sleep(rfport_rx_sample_stream, &sample);
	fifo_pop_or_sleep(rfport_rx_sample_stream, &sample);

	for (int c = 0; c != samples; c += 1) {
		int i;
		int q;

		dds_next_sample(mixer, &i, &q);
		fifo_pop_or_sleep(rfport_rx_sample_stream, &sample);

		if (sample.ch1 < rmin) {rmin = sample.ch1;}  // ref: ch1
		if (sample.ch1 > rmax) {rmax = sample.ch1;}  // ref: ch1
		if (sample.ch2 < mmin) {mmin = sample.ch2;}  // meas: ch2
		if (sample.ch2 > mmax) {mmax = sample.ch2;}  // meas: ch2

		if (window) {
			int w = raised_cos_window(c, samples);
			sample.ch1 = sample.ch1 * w / MAGNITUDE_CONST; // ref: ch1
			sample.ch2 = sample.ch2 * w / MAGNITUDE_CONST; // meas: ch2
		}

		m->ref_i += ((sample.ch1 * i) / MAGNITUDE_CONST);  // ref: ch1
		m->ref_q += ((sample.ch1 * q) / MAGNITUDE_CONST);  // ref: ch1
		m->meas_i += ((sample.ch2 * i) / MAGNITUDE_CONST);  // meas: ch2
		m->meas_q += ((sample.ch2 * q) / MAGNITUDE_CONST);  // meas: ch2
	}

	m->ref_ampl = rmax - rmin;
	m->meas_ampl = mmax - mmin;

	dds_destroy(mixer);

	rfport_rx_daq_off();
}
