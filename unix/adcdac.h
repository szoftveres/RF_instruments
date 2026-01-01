#ifndef INC_ADCDAC_H_
#define INC_ADCDAC_H_

#include <stdint.h>  // uint32
#include "os/fifo.h"




int dacsink_setup (fifo_t* in_stream);
int adcsrc_setup (fifo_t* out_stream, int fs);

#endif /* INC_ADCDAC_H_ */
