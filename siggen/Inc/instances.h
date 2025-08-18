#ifndef __INSTANCES_H__
#define __INSTANCES_H__

#include <bda4700.h>
#include "max2871.h"
#include "blockdevice.h"
#include "blockdev_fs.h"
#include "config.h"
#include "program.h"
#include "levelcal.h"
#include "parser.h"


#define PROGRAMS (8)

extern max2871_t *rf_pll;

extern bda4700_t *attenuator;

extern blockdevice_t *eeprom;

extern config_t config;

extern parser_t *online_parser;

extern direntry_t directory[PROGRAMS];

extern program_t* program;

extern int	program_ip;
extern int	program_run;


double set_rf_frequency (uint32_t khz);
void set_rf_output (int on);
int set_rf_level (int dBm);

void cfg_override (void);

int load_devicecfg (void);
int save_devicecfg (void);

void apply_cfg (void);
void print_cfg (void);

int direntries (void);

int execute_program (program_t *program);

#endif
