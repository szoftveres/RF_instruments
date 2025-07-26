#include "instances.h"
#include "functions.h"

max2871_t* rf_pll;

bda4700_t *attenuator;

blockdevice_t *eeprom;

config_t config;

parser_t *online_parser;

direntry_t directory[PROGRAMS];

program_t* program;

int	program_ip;
int	program_run;



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


int load_devicecfg (void) {
	blockfile_t *configfile = blockfile_create(eeprom, 0x00);
	if (!configfile) {
		return 0;
	}
	int rc = config_load(&config, configfile);
	if (rc) {
		apply_cfg();
		print_cfg();
	}
	blockfile_destroy(configfile);
	return rc;
}

int save_devicecfg (void) {
	blockfile_t *configfile = blockfile_create(eeprom, 0x00);
	if (!configfile) {
		return 0;
	}
	int rc = config_save(&config, configfile);
	blockfile_destroy(configfile);
	return rc;
}


void print_cfg (void) {
	console_printf("RF: %i kHz, %i dBm, output %s", config.fields.khz, config.fields.level, config.fields.rfon ? "on" : "off");
}


int direntries (void) {
	return (sizeof(directory)/sizeof(directory[0]));
}
