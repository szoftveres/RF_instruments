#ifndef INC_ADCDAC_H_
#define INC_ADCDAC_H_

#include <stdint.h>  // uint32
#include "../os/fifo.h"

void dac_sample_stream_callback (void* ctxt);
void adc1_sample_stream_callback (void* ctxt);

void dac1_outv (uint32_t aval);

int dac_max (void);
int dacsink_setup (fifo_t* in_stream);
int adcsrc_setup (fifo_t* out_stream, int fs);


#endif /* INC_ADCDAC_H_ */
