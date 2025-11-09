#ifndef __INSTANCES_H__
#define __INSTANCES_H__

#include <bda4700.h>
#include "max2871.h"
#include "blockdevice.h"
#include "fatsmall_fs.h"
#include "config.h"
#include "program.h"
#include "levelcal.h"
#include "parser.h"
#include "terminal_input.h"


#define PROGRAMS (8)

extern max2871_t *rf_pll;

extern bda4700_t *attenuator;

extern blockdevice_t *eeprom;

extern fs_t *eepromfs;

extern config_t config;

extern parser_t *online_parser;

extern program_t* program;

extern terminal_input_t* online_input;

extern int	program_ip;
extern int	program_run;
extern int  subroutine_stack[8];
extern int  subroutine_sp;


double set_rf_frequency (uint32_t khz);
void set_rf_output (int on);
int set_rf_level (int dBm);
int set_fs (int fs);
int set_fc (int fc);

void cfg_override (void);

int load_autorun_program (void);
int load_devicecfg (void);
int save_devicecfg (void);

void apply_cfg (void);
void print_cfg (void);

int execute_program (program_t *program);

int ticks_getter (void * context);

void frequency_setter (void * context, int khz);
int frequency_getter (void * context);
void rflevel_setter (void * context, int dBm);
int rflevel_getter (void * context);

void rnd_setter (void * context, int rand_set);
int rnd_getter (void * context);

#endif
