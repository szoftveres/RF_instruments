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


double set_rf_frequency (uint32_t khz);
void set_rf_output (int on);
int set_rf_level (int dBm);
int set_fs (int fs);
int set_fc (int fc);

void cfg_override (void);

void apply_cfg (void);
void print_cfg (void);


int baud_to_samples (int baud);

int transmit_data (uint8_t* data, int len);
int receive_data (uint8_t* data, int *len);

int setup_persona_commands (void);

#endif
