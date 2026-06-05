#ifndef __INSTANCES_H__
#define __INSTANCES_H__

#include <stdint.h>

int set_fs (int fs);
int set_fc (int fc);

int dac1_setter (void * context, int aval);

int baud_to_samples (int baud);

int transmit_data (uint8_t* data, int len);
int receive_data (uint8_t* data, int *len);

int setup_persona_commands (void);

#endif
