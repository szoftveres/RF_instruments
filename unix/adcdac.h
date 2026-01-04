#ifndef INC_ADCDAC_H_
#define INC_ADCDAC_H_

#include <stdint.h>  // uint32
#include "os/fifo.h"

int start_audio_in (int fs);
void stop_audio_in (void);
int start_audio_out (int fs);
void stop_audio_out (void);
int record_int16_sample (int16_t *s);
int play_int16_sample (int16_t *s);


int dacsink_setup (fifo_t* in_stream);
int adcsrc_setup (fifo_t* out_stream, int fs);

#endif /* INC_ADCDAC_H_ */
