#include "instances.h"
#include "functions.h"

max2871_t* osc;

bda4700_t *attenuator;

blockdevice_t *eeprom;

config_t config;
blockfile_t* configfile;

parser_t *online_parser;

program_t* program;
blockfile_t* programfile;

int	program_ip;
int	program_run;



double set_rf_frequency (uint32_t khz) {
	double actual_kHz;

	get_cal_range_index((int)khz);
	actual_kHz = max2871_freq(osc, khz);
	if (actual_kHz > 0) {
		config.fields.khz = khz;
	}
	return actual_kHz;
}


void set_rf_output (int on) {
	max2871_rfa_out(osc, on);
	config.fields.rfon = on;
	if (on) {
		ledon();
	} else {
		ledoff();
	}
}


int set_rf_level (int dBm) {
	//int freq_range = get_cal_range_index(config.fields.khz);
	if (!max2871_rfa_power(osc, -1)) {
		return 0;
	}
	if (!bda4700_set(attenuator, dBm * -1)) {
		return 0;
	}
	config.fields.level = dBm;
	return 1;
}

