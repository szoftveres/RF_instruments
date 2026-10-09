#include "rfport_rx.h"
#include "../os/dsp_maths.h"
#include "../os/hal_plat.h"
#include "../os/fifo.h"
#include <string.h> //memset
#include <limits.h> //int max and int min
#include "stm32h7xx_hal.h"


extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;


typedef struct rfport_rx_sample_s {
	int ch1;
	int ch2;
} rfport_rx_sample_t;


static fifo_t* rfport_rx_sample_stream;


void rfport_rx_daq_callback (void* ctxt) {
	rfport_rx_sample_t sample;

	sample.ch1 = ((int)hadc1.Instance->DR) - 32768;
	sample.ch2 = ((int)hadc2.Instance->DR) - 32768;

	HAL_ADC_Start(&hadc1);   //hadc1.Instance->CR |= (uint32_t)ADC_CR_ADSTART;
	HAL_ADC_Start(&hadc2);   //hadc2.Instance->CR |= (uint32_t)ADC_CR_ADSTART;

	fifo_push(rfport_rx_sample_stream, &sample);
}


void rfport_rx_daq_on (int fs) {
	rfport_rx_sample_stream = fifo_create(64, sizeof(rfport_rx_sample_t));

	set_sampler_frequency(fs);

	start_sampler(rfport_rx_daq_callback, NULL);
}


void rfport_rx_daq_off (void) {
	stop_sampler();

	fifo_destroy(rfport_rx_sample_stream);
}


void rfport_rx_meas (int fs, int fc, int samples, rfport_rx_t* m, int window) {
	dds_t* mixer;
	rfport_rx_sample_t sample;
	int mag = magnitude_const();
	int min = INT_MAX;
	int max = INT_MIN;

	rfport_rx_daq_on(fs);

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

		if (sample.ch1 < min) {min = sample.ch1;}  // ref: ch1
		if (sample.ch1 > max) {max = sample.ch1;}  // ref: ch1

		if (window) {
			int w = raised_cos_window(c, samples);
			sample.ch1 = sample.ch1 * w / mag; // ref: ch1
			sample.ch2 = sample.ch2 * w / mag; // meas: ch2
		}

		m->ref_i += ((sample.ch1 * i) / mag);  // ref: ch1
		m->ref_q += ((sample.ch1 * q) / mag);  // ref: ch1
		m->meas_i += ((sample.ch2 * i) / mag);  // meas: ch2
		m->meas_q += ((sample.ch2 * q) / mag);  // meas: ch2
	}

	m->ref_ampl = max - min;

	dds_destroy(mixer);

	rfport_rx_daq_off();
}

/* =================================================================== */

typedef struct bpsk_rf_s {
	int baudrate;
	int fs;
	int symbol_oversample_rate;

	moving_sum_t *i_hpf;
	moving_sum_t *q_hpf;

	moving_sum_t *symbol_filter;

	const int *nco_wavetable;
	uint8_t nco_phase;

	int sample_out;

} bpsk_rf_t;


bpsk_rf_t* bpsk_rf_create (int fs, int baudrate, int symbol_oversample_rate) {
	int hpf_symbols = 24;
	bpsk_rf_t *instance = (bpsk_rf_t*)t_malloc(sizeof(bpsk_rf_t));

	instance->i_hpf = moving_sum_create((fs / baudrate) * hpf_symbols);
	instance->q_hpf = moving_sum_create((fs / baudrate) * hpf_symbols);
	instance->symbol_filter = moving_sum_create((fs / baudrate));

	instance->fs = fs;
	instance->baudrate = baudrate;
	instance->symbol_oversample_rate = symbol_oversample_rate;
	instance->nco_wavetable = sinewave; // 256 long
	return instance;
}


void bpsk_rf_destroy (bpsk_rf_t *instance) {
	moving_sum_destroy(instance->i_hpf);
	moving_sum_destroy(instance->q_hpf);
	moving_sum_destroy(instance->symbol_filter);
	t_free(instance);

}


void rfport_rx_bpsk_smple (bpsk_rf_t *instance) {
	rfport_rx_sample_t sample;
	int mag = magnitude_const();
	int dec = (instance->fs / instance->baudrate) / instance->symbol_oversample_rate; // 160000 / 2000 / 5

	while (dec) {
		int nco_i = instance->nco_wavetable[(instance->nco_phase) & 0xFF];
		int nco_q = instance->nco_wavetable[(instance->nco_phase + 0x40) & 0xFF];

		fifo_pop_or_sleep(rfport_rx_sample_stream, &sample);

		int i_rot = sample.ch1 - moving_average(instance->i_hpf, sample.ch1); // DC average level removal
		int q_rot = sample.ch2 - moving_average(instance->q_hpf, sample.ch2); // DC average level removal

		int i = ((i_rot * nco_i) + (q_rot * nco_q)) / mag;   // phase matrix
		int q = ((i_rot * nco_q) + (q_rot * nco_i)) / mag;   // phase matrix

		instance->nco_phase += (i * q) > 0 ? (uint8_t) 0x10 : (uint8_t) -0x10;  // NCO tuning, +16 or -16

		instance->sample_out = moving_average(instance->symbol_filter, i);  // symbol matched filter
		dec -= 1;
	}
}


void rfport_rx_bpsk (void) {
	int fs = 160000;
	int baudrate = 2000;
	int oversample = 5;

	bpsk_rf_t* bpsk_rfmodem = bpsk_rf_create (fs, baudrate, oversample);
	rfport_rx_daq_on(fs);

	for (int i = 0; i != 10000; i++) {
		rfport_rx_bpsk_smple(bpsk_rfmodem);

	}

	rfport_rx_daq_off();
	bpsk_rf_destroy(bpsk_rfmodem);
}
