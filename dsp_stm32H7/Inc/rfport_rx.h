#ifndef INC_RFPORT_RX_H_
#define INC_RFPORT_RX_H_


typedef struct rfport_rx_s {
	int ref_i;
	int ref_q;
	int ref_ampl;
	int meas_i;
	int meas_q;
} rfport_rx_t;


void rfport_rx_meas (int fc, int samples, rfport_rx_t* m);


#endif /* INC_RFPORT_RX_H_ */
