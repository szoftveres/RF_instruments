#ifndef __INSTANCES_H__
#define __INSTANCES_H__

#include "max2871.h"
#include "attenuator.h"
#include "config.h"
#include "levelcal.h"
#include "parser.h"

#define EEPROM_CONFIG_ADDRESS (0x00)


extern max2871_instance_t *osc;

extern attenuator_instance_t *attenuator;

extern config_t config;

extern parser_t *online_parser;

extern int	program_ip;
extern int	program_run;


double set_rf_frequency (uint32_t khz);
void set_rf_output (int on);
int set_rf_level (int dBm);

#endif
