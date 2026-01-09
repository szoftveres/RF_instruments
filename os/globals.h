#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include "fatsmall_fs.h"
#include "config.h"
#include "program.h"
#include "levelcal.h"
#include "dsp_maths.h"
#include "terminal_input.h"
#include "taskscheduler.h"
#include "fifo.h" // execute_program


extern config_t config;

extern program_t* program;

extern terminal_input_t* online_input;

extern taskscheduler_t *scheduler;

extern fs_broker_t *fs;


extern int	program_ip;
extern int	program_run;
extern int  subroutine_stack[8];
extern int  subroutine_sp;


int load_autorun_program (void);
int load_devicecfg (void);
int save_devicecfg (void);

void global_cfg_override (void);

int execute_program (program_t *program, fifo_t* in, fifo_t* out);

int cmd_fsinfo (void);


#endif
