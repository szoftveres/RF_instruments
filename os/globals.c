#include "globals.h"
#include "analog.h"
#include <string.h> // memcpy
#include <stdio.h> // EOF
#include "hal_plat.h" // malloc free
#include "resource.h"
#include "parser.h" // execute program
#include "fs_broker.h"


config_t config;

program_t* program;

terminal_input_t* online_input;

taskscheduler_t *scheduler;

fs_broker_t *fs;


int	program_ip;
int	program_run;
int subroutine_stack[8];
int subroutine_sp;


int load_autorun_program (void) {
	int rc;
	int fd;
	fd = open_f(fs, "autoprg", FS_O_READONLY);
	if (fd < 0) {
		return 0;
	}
	rc = program_load(program, fs, fd);
	close_f(fs, fd);

	if (rc < 1) {
		rc = 0;
	}
	return rc;
}


int load_devicecfg (void) {
	config_t lcl_config;
	int rc;
	int fd;
	fd = open_f(fs, "cfg", FS_O_READONLY);
	if (fd < 0) {
		return 0;
	}

	rc = config_load(&lcl_config, fs, fd);
	close_f(fs, fd);

	if (rc < 1) {
		rc = 0;
	}

	if (rc) {
		memcpy(&config, &lcl_config, (sizeof(config_t)));
		global_cfg_override();
	}
	return rc;
}

int save_devicecfg (void) {
	int fd;
	int rc;

	fd = open_f(fs, "cfg", FS_O_CREAT | FS_O_TRUNC);
	if (fd < 0) {
		console_printf("conf save:open fail");
		return 0;
	}

	rc = config_save(&config, fs, fd);
	close_f(fs, fd);
	if (rc < 1) {
		rc = 0;
	}
	return rc;
}



int execute_program (program_t *program, fifo_t* in, fifo_t* out) {
	int rc = 1;
	program_ip = 0;
	program_run = 1;
	char* line;

	if (in) {
		in->readers++;
	}
	if (out) {
		out->writers++;
	}

	while (program_run) {
		if (switchbreak()) {
			program_run = 0;
			console_printf("Break");
			break;
		}

		line = program_line(program, program_ip);
		program_ip += 1;

		parser_t *lcl_parser = parser_create(program->header.fields.linelen); // line lenght is of the program's
		if (!lcl_parser) {
			program_run = 0;
			rc = 0;
			break;
		}
		rc = cmd_line_parser(lcl_parser, line, in, out);
		if (!rc) {
			program_run = 0;
		}

		parser_destroy(lcl_parser);

		if (program_ip >= program->header.fields.nlines) {
			program_run = 0;
			console_printf("Done");
			break;
		}
	}

	if (in) {
		in->readers--;
	}
	if (out) {
		out->writers--;
	}

	return rc;
}



int cmd_fsinfo (void) {
	//int n;
	return 1;
	//fs_dump_fat(eepromfs);
	//console_printf("file_entry_t %i", sizeof(file_entry_t));
	//console_printf("device_params_t %i", sizeof(device_params_t));
	leading_wspace(0, 20);
	//console_printf("%i entries free", fs_count_empyt_direntries(eepromfs));
	//n = fs_count_empyt_blocks(eepromfs);
	leading_wspace(0, 20);
	//console_printf("%i blocks (%i Bytes) free", n, (n * eepromfs->device->blocksize));
	return 1;
}

