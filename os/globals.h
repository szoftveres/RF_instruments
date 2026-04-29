#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include "fs_broker.h"
#include "config.h"
#include "program.h"
#include "levelcal.h"
#include "dsp_maths.h"
#include "taskscheduler.h"
#include "fifo.h" // execute_program
#include <stdarg.h>  // console_printf



extern stdio_stack_t *iostack;
#define STDIN (iostack->fin)
#define STDOUT (iostack->fout)
#define STDERR (iostack->ferr)

extern config_t config;

extern program_t* program;

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

int nullfile_read (struct generic_file_s *file, int count, char* buf);
int nullfile_write (struct generic_file_s *file, int count, char* buf);
int nullfile_open (struct generic_file_s *file, int flags);
void nullfile_close (struct generic_file_s *file);

int streamfile_read (struct generic_file_s* context, int b, char* buf);
int streamfile_write (struct generic_file_s* context, int b, char* buf);

void command_line_loop ();

int printf_f (int fd, const char* fmt, ...);


#endif
