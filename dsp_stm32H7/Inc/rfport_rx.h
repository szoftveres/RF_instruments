#ifndef INC_RFPORT_RX_H_
#define INC_RFPORT_RX_H_

#include "../os/fifo.h"

typedef struct rfport_rx_s {
	int ref_i;
	int ref_q;
	int ref_ampl;
	int meas_i;
	int meas_q;
	int meas_ampl;
} rfport_rx_t;


void rfport_rx_meas (int fs, int fc, int samples, rfport_rx_t* m, int window);


typedef struct rfport_rx_sample_s {
	int ch1;
	int ch2;
} rfport_rx_sample_t;

void rfport_rx_daq_on (int fs, int bufdepth);
void rfport_rx_daq_off (void);

extern fifo_t* rfport_rx_sample_stream;

#endif /* INC_RFPORT_RX_H_ */
