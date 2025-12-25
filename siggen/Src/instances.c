#include "instances.h"
#include "analog.h"
#include <string.h> // memcpy
#include <stdio.h> // EOF
#include "hal_plat.h" // malloc free
#include "resource.h"
#include "parser.h" // execute program

max2871_t* rf_pll;

bda4700_t *attenuator;

fs_t *eepromfs;

config_t config;

program_t* program;

terminal_input_t* online_input;

taskscheduler_t *scheduler;


int	program_ip;
int	program_run;
int subroutine_stack[8];
int subroutine_sp;

static const char* invalid_val = "Invalid value \'%i\'";


double set_rf_frequency (uint32_t khz) {
	double actual_kHz;

	get_cal_range_index((int)khz);
	actual_kHz = max2871_freq(rf_pll, khz);
	if (actual_kHz > 0) {
		config.fields.khz = khz;
	}
	return actual_kHz;
}


void set_rf_output (int on) {
	max2871_rfa_out(rf_pll, on);
	config.fields.rfon = on;
	if (on) {
		ledon();
	} else {
		ledoff();
	}
}


int set_rf_level (int dBm) {
	//int freq_range = get_cal_range_index(config.fields.khz);
	if (!max2871_rfa_power(rf_pll, -1)) {
		return 0;
	}
	if (!bda4700_set(attenuator, dBm * -1)) {
		return 0;
	}
	config.fields.level = dBm;
	return 1;
}


void apply_cfg (void) {
	set_rf_level(config.fields.level);
	set_rf_frequency(config.fields.khz);
	set_rf_output(config.fields.rfon);
}


void cfg_override (void) {
	config.fields.rfon = 1; // always load with RF on
	set_rf_output(config.fields.rfon);
}


int load_autorun_program (void) {
	int rc;
	int fd;
	fd = fs_open(eepromfs, "autoprg", FS_O_READONLY);
	if (fd < 0) {
		return 0;
	}
	rc = program_load(program, eepromfs, fd);
	fs_close(eepromfs, fd);

	if (rc < 1) {
		rc = 0;
	}
	return rc;
}


int load_devicecfg (void) {
	config_t lcl_config;
	int rc;
	int fd;
	fd = fs_open(eepromfs, "cfg", FS_O_READONLY);
	if (fd < 0) {
		return 0;
	}

	rc = config_load(&lcl_config, eepromfs, fd);
	fs_close(eepromfs, fd);

	if (rc < 1) {
		rc = 0;
	}

	if (rc) {
		memcpy(&config, &lcl_config, (sizeof(config_t)));
		cfg_override();
		apply_cfg();
		print_cfg();
	}
	return rc;
}

int save_devicecfg (void) {
	int fd;
	int rc;

	fd = fs_open(eepromfs, "cfg", FS_O_CREAT | FS_O_TRUNC);
	if (fd < 0) {
		console_printf("conf save:open fail");
		return 0;
	}

	rc = config_save(&config, eepromfs, fd);
	fs_close(eepromfs, fd);
	if (rc < 1) {
		rc = 0;
	}
	return rc;
}


void print_cfg (void) {
	console_printf("RF: %i kHz, %i dBm, output %s", config.fields.khz, config.fields.level, config.fields.rfon ? "on" : "off");
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


int frequency_setter (void * context, int khz) {
	double actual = set_rf_frequency(khz);
	int khzpart = (int)actual;
	int hzpart = (actual - (double)khzpart) * 1000.0;
	int error = (khz - actual) * 1000.0;
	if (actual < 0) {
		console_printf(invalid_val, khz);
		return 0;
	}
	console_printf("actual: %i.%03i kHz, error: %i Hz", khzpart, hzpart, error);
	print_cfg();
	return 1;
}

int frequency_getter (void * context) {
	return config.fields.khz;
}

int rflevel_setter (void * context, int dBm) {
	if (!set_rf_level(dBm)) {
		console_printf(invalid_val, dBm);
		return 0;
	}
	print_cfg();
	return 1;
}

int rflevel_getter (void * context) {
	return config.fields.level;
}



int fs_setter (void * context, int fs) {
	if (!set_fs(fs)) {
		console_printf(invalid_val, fs);
		return 0;
	}
	console_printf("fs: %i Hz", fs);
	return 1;
}

int fc_setter (void * context, int fc) {
	if (!set_fc(fc)) {
		console_printf(invalid_val, fc);
		return 0;
	}
	console_printf("fc: %i Hz", fc);
	return 1;
}

int dac1_setter (void * context, int aval) {
	resource_t* resource = (resource_t*)context;
	if (aval < 0 || aval >= dac_max()) {
		console_printf(invalid_val, aval);
		return 0;
	}
	resource->value = aval;
	dac1_outv((uint32_t)aval);
	return 1;
}

