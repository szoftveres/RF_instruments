#include "rfport_rx.h"
#include "../os/dsp_maths.h"
#include "../os/hal_plat.h"
#include "../os/fifo.h"
#include "main.h" // GPIO custom names
#include <string.h> //memset
#include <limits.h> //int max and int min
#include "../os/globals.h" // stdio
#include "stm32h7xx_hal.h"
#include "bpsk_transceiver.h"

/*-----------------------------------------------------*/

static fifo_t* bpsk_rf_tx_bit_stream;


void bpsk_rf_tx_stream_callback (void* ctxt) {
	int bit = 0;
	fifo_pop(bpsk_rf_tx_bit_stream, &bit);
	HAL_GPIO_WritePin(CHANNEL_SELECT_GPIO_Port, CHANNEL_SELECT_Pin, bit ? GPIO_PIN_SET : GPIO_PIN_RESET);
}


void bpsk_rf_tx_stream_on (int fs, int bufdepth) {
	bpsk_rf_tx_bit_stream = fifo_create(bufdepth, sizeof(int));

	set_sampler_frequency(fs);
	start_sampler(bpsk_rf_tx_stream_callback, NULL);
}


void bpsk_rf_tx_stream_off (void) {
	stop_sampler();
	fifo_destroy(bpsk_rf_tx_bit_stream);
}

/*-----------------------------------------------------*/

#define BPSK_RF_PKT_TOTAL_LEN(l) (((int)(l)) + ((int)sizeof(bpsk_rf_pkt_header_t)))
#define BPSK_RF_PREAMBLE (0xD9C2)


bpsk_rx_t* bpsk_rf_create (int fs, int baudrate, int oversample) {
	int hpf_symbols = 96;
	bpsk_rx_t *instance = (bpsk_rx_t*)t_malloc(sizeof(bpsk_rx_t));

	instance->i_hpf = moving_sum_create((fs / baudrate) * hpf_symbols);
	instance->q_hpf = moving_sum_create((fs / baudrate) * hpf_symbols);
	instance->symbol_filter = moving_sum_create((fs / baudrate));

	instance->preamble = (uint16_t*)t_malloc(sizeof(uint16_t) * oversample);
	instance->preamble_ampl = (moving_sum_t**)t_malloc(sizeof(moving_sum_t*) * oversample);
	for (int i = 0; i != oversample; i++) {
		instance->preamble_ampl[i] = moving_sum_create(16); // preamble is 16 bits long
	}

	instance->fs = fs;
	instance->baudrate = baudrate;
	instance->oversample = oversample;
	instance->nco_wavetable = sinewave; // 256 long
	return instance;
}


void bpsk_rf_destroy (bpsk_rx_t *instance) {
	moving_sum_destroy(instance->i_hpf);
	moving_sum_destroy(instance->q_hpf);
	moving_sum_destroy(instance->symbol_filter);
	for (int i = 0; i != instance->oversample; i++) {
		moving_sum_destroy(instance->preamble_ampl[i]);
	}
	t_free(instance->preamble_ampl);
	t_free(instance->preamble);
	t_free(instance);

}


// Receiving one slice of an oversampled symbol
int rx_bpsk_sample (bpsk_rx_t *instance) {
	int sample_out;
	rfport_rx_sample_t sample;
	int dec = (instance->fs / instance->baudrate) / instance->oversample; // 160000 / 2000 / 5 = 16

	while (dec) {
		int nco_i = instance->nco_wavetable[(instance->nco_phase) & 0xFF];
		int nco_q = instance->nco_wavetable[(instance->nco_phase + 0x40) & 0xFF];

		fifo_pop_or_sleep(rfport_rx_sample_stream, &sample);

		// DC average level removal
		int i_rot = sample.ch1 - moving_average(instance->i_hpf, sample.ch1);
		int q_rot = sample.ch2 - moving_average(instance->q_hpf, sample.ch2);

		// complex multiplication
		int i = ((i_rot * nco_i) - (q_rot * nco_q)) / MAGNITUDE_CONST;
		int q = ((i_rot * nco_q) + (q_rot * nco_i)) / MAGNITUDE_CONST;

		// Costas-loop NCO tuning, +16 or -16
		instance->nco_phase += (i * q) > 0 ? ((uint8_t) 0x10) : ((uint8_t) -0x10);

		// symbol matched filter
		sample_out = moving_average(instance->symbol_filter, i);
		dec -= 1;
	}
	return sample_out;
}



#define BPSK_RF_BAUDRATE (2000)
#define BPSK_RF_OVERSAMPLE (5)


int rf_bpsk_rx (char *payload, int *len, int* pwr) {
	int fs = 160000;
	uint8_t scrambler_core = 0xFF;
	int baudrate = BPSK_RF_BAUDRATE;
	int oversample = BPSK_RF_OVERSAMPLE;
	int ampl;
	int sign;
	int rc = 1;
	int acquire = 1;

	bpsk_rx_t* bpsk_rfmodem = bpsk_rf_create(fs, baudrate, oversample);

	for (int i = 0; i != bpsk_rfmodem->oversample; i++) {
		bpsk_rfmodem->preamble[i] = 0;
	}

	rfport_rx_daq_on(fs, 160000 / baudrate);

	int statemachine = 1;
	int oversampl_idx = 0;

	int max_ampl;
	int max_idx;

	uint16_t curr_preamble = 0;

	// Acquiring and locking onto the preamble, determining the absolute sign
	while (acquire) {
		bpsk_rfmodem->preamble[oversampl_idx] = curr_preamble;
		oversampl_idx = (oversampl_idx + 1) % oversample;
		curr_preamble = bpsk_rfmodem->preamble[oversampl_idx] << 1;

		int sample = rx_bpsk_sample(bpsk_rfmodem);

		curr_preamble |= ((sample > 0) ? 1 : 0);
		ampl = moving_average(bpsk_rfmodem->preamble_ampl[oversampl_idx], ((sample > 0) ? sample : -sample));

		switch (statemachine) {
		case 1:
			if (curr_preamble == (uint16_t)BPSK_RF_PREAMBLE) {
				max_ampl = ampl;
				max_idx = oversampl_idx;
				statemachine += 1;
				sign = 1;
			} else if (curr_preamble == (uint16_t)(~(BPSK_RF_PREAMBLE))) {
				max_ampl = ampl;
				max_idx = oversampl_idx;
				statemachine += 1;
				sign = 0;
			}
			break;

		case 2:
			if (curr_preamble != (sign ? (uint16_t)BPSK_RF_PREAMBLE : (uint16_t)(~(BPSK_RF_PREAMBLE)))) {
				acquire = 0;
				break;
			}
			if (ampl > max_ampl) {
				max_ampl = ampl;
				max_idx = oversampl_idx;
			} else {
				acquire = 0;
			}
			break;
		}
	}

	bpsk_rf_pkt_t pkt;
	pkt.h.len = BPSK_RF_PKT_PAYLOAD_MAX;

	int bytep = 0;
	int bitn = 0;
	uint8_t byte;

	// Receiving the packet
	while (bytep < (BPSK_RF_PKT_TOTAL_LEN(pkt.h.len))) {
		int sample;
		do {
			oversampl_idx = (oversampl_idx + 1) % oversample;
			sample = rx_bpsk_sample(bpsk_rfmodem);
		} while (oversampl_idx != max_idx);

		byte <<= 1;
		byte |= ((sample > 0) ? 1 : 0);

        bitn += 1;
        if (bitn == 8) {
        	 bitn = 0;

        	 pkt.byte[bytep] = scrambler8(&scrambler_core, sign ? byte : (~byte));
        	 bytep += 1;

        	 if (pkt.h.len > BPSK_RF_PKT_PAYLOAD_MAX) {
        		 rc = 0; /* len got corrupted */
        		 break;
        	 }
        }
	}

	rfport_rx_daq_off();
	bpsk_rf_destroy(bpsk_rfmodem);

	if (rc) {
		uint16_t crc = pkt.h.crc16;
		pkt.h.crc16 = 0;
		if ((!crc) || (crc16(pkt.byte, BPSK_RF_PKT_TOTAL_LEN(pkt.h.len)) != crc)) {
			rc = 0;
		}
	}
	if (rc) {
		memcpy(payload, pkt.payload, pkt.h.len);
		*len = (int)pkt.h.len;
		*pwr = ampldb(max_ampl, 65536);
	}
	return rc;
}


int rf_bpsk_tx (char* payload, int len) {
	uint8_t scrambler_core = 0xFF;

	if ((len < 0) || (len > BPSK_RF_PKT_PAYLOAD_MAX)) {
		return 0;
	}

	bpsk_rf_pkt_t pkt;
	pkt.h.len = (uint8_t)len;
	memcpy(pkt.payload, payload, len);
	pkt.h.crc16 = 0;
	pkt.h.crc16 = crc16(pkt.byte, BPSK_RF_PKT_TOTAL_LEN(pkt.h.len));

	bpsk_rf_tx_stream_on(BPSK_RF_BAUDRATE, 24);

	/* Sending some symbols, to give the Rx Costas loop something to lock onto */
	for (int i = 0; i != 48; i++) {
		int bit = (i % 2);
		fifo_push_or_sleep(bpsk_rf_tx_bit_stream, &bit);
	}

	// Sending preamble
	uint16_t preamble = BPSK_RF_PREAMBLE;
	for (int i = 0; i != 16; i++) {
		int bit = (preamble & 0x8000) ? 1 : 0;
		fifo_push_or_sleep(bpsk_rf_tx_bit_stream, &bit);
		preamble <<= 1;
	}

	int bytep = 0;
	int bitn = 0;
	uint8_t byte = scrambler8(&scrambler_core, pkt.byte[bytep]);
	// Sending the packet
	while (bytep <= (BPSK_RF_PKT_TOTAL_LEN(pkt.h.len) + 1)) {
		int bit = (byte & 0x80) ? 1 : 0;
        fifo_push_or_sleep(bpsk_rf_tx_bit_stream, &bit);

        byte <<= 1;
        bitn += 1;
        if (bitn == 8) {
        	 bitn = 0;
        	 bytep += 1;
        	 byte = scrambler8(&scrambler_core, pkt.byte[bytep]);
        }
	}

	while (!fifo_isempty(bpsk_rf_tx_bit_stream)) {cpu_sleep();}

	bpsk_rf_tx_stream_off();
	return 1;
}
