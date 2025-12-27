#ifndef __INSTANCES_H__
#define __INSTANCES_H__

#include "../os/bda4700.h"
#include "../os/max2871.h"
#include "../os/config.h"
#include "../os/levelcal.h"
#include "../os/fatsmall_fs.h"

extern max2871_t *rf_pll;

extern bda4700_t *attenuator;

extern fs_t *eepromfs;  // FORMAT



int set_fs (int fs);
int set_fc (int fc);

void cfg_override (void);

void apply_cfg (void);
void print_cfg (void);

int fs_setter (void * context, int fs);
int fs_getter (void * context);
int fc_setter (void * context, int fc);
int fc_getter (void * context);

int dac1_setter (void * context, int aval);

int baud_to_samples (int baud);

int transmit_data (uint8_t* data, int len);
int receive_data (uint8_t* data, int *len);

int setup_persona_commands (void);

#endif
