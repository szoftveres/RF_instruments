#ifndef __INSTANCES_H__
#define __INSTANCES_H__

#include "os/bda4700.h"
#include "os/max2871.h"
#include "os/config.h"
#include "os/levelcal.h"


extern max2871_t *rf_pll;

extern bda4700_t *attenuator;

double set_rf_frequency (uint32_t khz);
void set_rf_output (int on);
int set_rf_level (int dBm);
int set_fs (int fs);
int set_fc (int fc);

void cfg_override (void);

void apply_cfg (void);
void print_cfg (void);


int frequency_setter (void * context, int khz);
int frequency_getter (void * context);
int rflevel_setter (void * context, int dBm);
int rflevel_getter (void * context);
int fs_setter (void * context, int fs);
int fs_getter (void * context);
int fc_setter (void * context, int fc);
int fc_getter (void * context);

int dac1_setter (void * context, int aval);

int baud_to_samples (int baud);

int transmit_data (uint8_t* data, int len);
int receive_data (uint8_t* data, int *len);

#endif
