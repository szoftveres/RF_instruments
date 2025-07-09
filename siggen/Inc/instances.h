#ifndef __INSTANCES_H__
#define __INSTANCES_H__

#include <bda4700.h>
#include "max2871.h"
#include "blockdevice.h"
#include "blockfile.h"
#include "config.h"
#include "program.h"
#include "levelcal.h"
#include "parser.h"


extern max2871_t *osc;

extern bda4700_t *attenuator;

extern blockdevice_t *eeprom;

extern config_t config;
extern blockfile_t* configfile;

extern parser_t *online_parser;

extern program_t* program;
extern blockfile_t* programfile;

extern int	program_ip;
extern int	program_run;


double set_rf_frequency (uint32_t khz);
void set_rf_output (int on);
int set_rf_level (int dBm);

void print_cfg (void) ;

#endif
