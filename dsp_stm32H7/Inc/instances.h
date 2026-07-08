#ifndef __INSTANCES_H__
#define __INSTANCES_H__

#include "../bda4700/bda4700.h"
#include "../max2871/max2871.h"
#include "../os/fatsmall_fs.h"

extern max2871_t *rf_pll;
extern max2871_t *lo_pll;
extern bda4700_t *attenuator;

extern fs_t *eepromfs;  // FORMAT

int dac1_setter (void * context, int aval);


int transmit_data (uint8_t* data, int len);
int receive_data (uint8_t* data, int *len);

int setup_persona_commands (void);

#endif
