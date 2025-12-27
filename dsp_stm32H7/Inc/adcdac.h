#ifndef INC_ADCDAC_H_
#define INC_ADCDAC_H_

#include <stdint.h>  // uint32
#include "../os/fifo.h"


void dac1_outv (uint32_t aval);
int adc1_getter (void * context);
int adc2_getter (void * context);
int adc3_getter (void * context);

int dac_max (void);
int dacsink_setup (fifo_t* in_stream);

#endif /* INC_ADCDAC_H_ */
