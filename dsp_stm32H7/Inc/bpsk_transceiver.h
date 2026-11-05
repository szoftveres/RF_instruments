#ifndef INC_BPSK_TRANSCEIVER_H_
#define INC_BPSK_TRANSCEIVER_H_


#include "../os/dsp_maths.h"  // moving_sum

#define BPSK_RF_PKT_PAYLOAD_MAX (64)




typedef struct __attribute__((packed)) {
    uint8_t len;
    uint16_t crc16;
} bpsk_rf_pkt_header_t;


typedef union __attribute__((packed)) bpsk_rf_pkt_u {
    uint8_t byte[0];
    struct __attribute__((packed)) {
    	bpsk_rf_pkt_header_t h;
        char payload[BPSK_RF_PKT_PAYLOAD_MAX];
    };
} bpsk_rf_pkt_t;


typedef struct bpsk_rx_s {
	int baudrate;
	int fs;
	int oversample;

	moving_sum_t *i_hpf;
	moving_sum_t *q_hpf;

	moving_sum_t *symbol_filter;

	const int *nco_wavetable;
	uint8_t nco_phase;

	int sample_out;
	uint16_t *preamble;
	moving_sum_t **preamble_ampl;
	bpsk_rf_pkt_t pkt;

} bpsk_rx_t;


bpsk_rx_t* bpsk_rf_create (int fs, int baudrate, int oversample);
void bpsk_rf_destroy (bpsk_rx_t *instance);

int rfport_tx_bpsk (char* payload, int len);

#endif /* INC_BPSK_TRANSCEIVER_H_ */
