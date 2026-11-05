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


#define BPSK_RF_PKT_TOTAL_LEN(l) (((int)(l)) + ((int)sizeof(bpsk_rf_pkt_header_t)))
#define BPSK_RF_PREAMBLE (0xD9C2)



bpsk_rx_t* bpsk_rf_create (int fs, int baudrate, int oversample) {
	int hpf_symbols = 24;
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


void rfport_rx_bpsk_smple (bpsk_rx_t *instance) {
	rfport_rx_sample_t sample;
	int dec = (instance->fs / instance->baudrate) / instance->oversample; // 160000 / 2000 / 5 = 16

	while (dec) {
		int nco_i = instance->nco_wavetable[(instance->nco_phase) & 0xFF];
		int nco_q = instance->nco_wavetable[(instance->nco_phase + 0x40) & 0xFF];

		fifo_pop_or_sleep(rfport_rx_sample_stream, &sample);

		int i_rot = sample.ch1 - moving_average(instance->i_hpf, sample.ch1); // DC average level removal
		int q_rot = sample.ch2 - moving_average(instance->q_hpf, sample.ch2); // DC average level removal

		int i = ((i_rot * nco_i) - (q_rot * nco_q)) / MAGNITUDE_CONST;   // complex multiplication
		int q = ((i_rot * nco_q) + (q_rot * nco_i)) / MAGNITUDE_CONST;   // complex multiplication

		instance->nco_phase += (i * q) > 0 ? (uint8_t) 0x10 : (uint8_t) -0x10;  // NCO tuning, +16 or -16

		instance->sample_out = moving_average(instance->symbol_filter, i);  // symbol matched filter
		dec -= 1;
	}
}


int rfport_rx_bpsk (void) {
	int fs = 160000;
	uint8_t scrambler_core = 0xFF;
	int baudrate = 2000;
	int oversample = 5;
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

	while (acquire) { /* Acquiring the preamble */
		oversampl_idx = (oversampl_idx + 1) % oversample;
		bpsk_rfmodem->preamble[oversampl_idx] <<= 1;

		rfport_rx_bpsk_smple(bpsk_rfmodem);
		int sample = bpsk_rfmodem->sample_out;

		bpsk_rfmodem->preamble[oversampl_idx] |= ((sample > 0) ? 1 : 0);
		ampl = moving_average(bpsk_rfmodem->preamble_ampl[oversampl_idx], ((sample > 0) ? sample : -sample));

		switch (statemachine) {
		case 1:
			if (bpsk_rfmodem->preamble[oversampl_idx] == BPSK_RF_PREAMBLE) {
				max_ampl = ampl;
				max_idx = oversampl_idx;
				statemachine += 1;
				sign = 1;
			} else if (bpsk_rfmodem->preamble[oversampl_idx] == (~(BPSK_RF_PREAMBLE))) {
				max_ampl = ampl;
				max_idx = oversampl_idx;
				statemachine += 1;
				sign = 0;
			}
			break;

		case 2:
			if (bpsk_rfmodem->preamble[oversampl_idx] != (sign ? BPSK_RF_PREAMBLE : (~(BPSK_RF_PREAMBLE)))) {
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

	while (bytep < (BPSK_RF_PKT_TOTAL_LEN(pkt.h.len))) {  /* Receiving the packet */

		oversampl_idx = (oversampl_idx + 1) % oversample;
		rfport_rx_bpsk_smple(bpsk_rfmodem);

		if (oversampl_idx != max_idx) {
			continue;
		}

		byte <<= 1;
		byte |= ((bpsk_rfmodem->sample_out > 0) ? 1 : 0);

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
		console_printf_e("[-%i dB, %i, %c], ", ampldb(max_ampl, 32768), pkt.h.len, sign?'+':'-');
		console_printf("%s", pkt.payload);
	}
	return rc;
}


int rfport_tx_bpsk (char* payload, int len) {
	uint8_t scrambler_core = 0xFF;

	if ((len < 0) || (len > BPSK_RF_PKT_PAYLOAD_MAX)) {
		return 0;
	}

	bpsk_rf_pkt_t pkt;
	pkt.h.len = (uint8_t)len;
	memcpy(pkt.payload, payload, len);
	pkt.h.crc16 = 0;
	pkt.h.crc16 = crc16(pkt.byte, BPSK_RF_PKT_TOTAL_LEN(pkt.h.len));

	bpsk_rf_tx_stream_on(2000, 24);

	/* Sending some symbols, to give the Rx Costas loop something to lock onto */
	for (int i = 0; i != 24; i++) {
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
