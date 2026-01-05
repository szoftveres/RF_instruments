#include "instances.h"
#include "config_def.h"
#include "../os/globals.h"  // config instance
#include <string.h> // memcpy
#include <stdio.h> // EOF
#include "../os/hal_plat.h" // malloc free
#include "../os/keyword.h" // command structure

#include "stm32f4xx_hal.h" // HAL_GetTick


max2871_t* rf_pll;

bda4700_t *attenuator;

fs_t *eepromfs; // FORMAT

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



void global_cfg_override (void) {
	cfg_override();
	apply_cfg();
	print_cfg();
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



void print_cfg (void) {
	console_printf("RF: %i kHz, %i dBm, output %s", config.fields.khz, config.fields.level, config.fields.rfon ? "on" : "off");
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


/*
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
*/



/*-----------------------------------------*/

#include <stdlib.h>

int cmd_malloctest (cmd_param_t** params __attribute__((unused)), fifo_t* in __attribute__((unused)), fifo_t* out __attribute__((unused))) {
	int i = 0;
	const int size = 1024;
	while (malloc(size)) {
		i++;
	}
	console_printf("size:%i, total:%i", size, size*i);
	cpu_halt();
	return 1;
}


#include "../os/fatsmall_fs.h"
int cmd_format (cmd_param_t** params __attribute__((unused)), fifo_t* in __attribute__((unused)), fifo_t* out __attribute__((unused))) {
	char* line = terminal_get_line(online_input, " type \"yes\"> ", 1);
	if (strcmp(line, "yes")) {
		console_printf("aborted");
		return 1;
	}
	fs_format(eepromfs, 16);
	return cmd_fsinfo();
}


int cmd_sleep (cmd_param_t** params, fifo_t* in __attribute__((unused)), fifo_t* out __attribute__((unused))) {
	int ms;

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_NUM) {
		return 0;
	}
	ms = (*params)->n;
	cmd_param_consume(params);

	if (ms < 0 || ms > 3600000) { // max 1 hour
		console_printf(invalid_val, ms);
		return 0;
	}

	uint32_t tickstart = HAL_GetTick();

	while((HAL_GetTick() - tickstart) < ms) {
		if (switchbreak()) {
			console_printf("Break");
			break;
		}
	}
	return 1;
}



int cmd_amtone (cmd_param_t** params, fifo_t* in __attribute__((unused)), fifo_t* out __attribute__((unused))) {
	int ms;

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_NUM) {
		return 0;
	}
	ms = (*params)->n;
	cmd_param_consume(params);

	if (ms < 0 || ms > 3600000) { // max 1 hour
		console_printf(invalid_val, ms);
		return 0;
	}

	uint32_t tickstart = HAL_GetTick();
	uint32_t thistick;
	int level = rflevel_getter(NULL);
	int state = 0;

	while (((thistick = HAL_GetTick()) - tickstart) < ms) {
		if (switchbreak()) {
			console_printf("Break");
			break;
		}
		if (!set_rf_level(state ? -30 : level)) {
			break;
		}
		state += 1; state %= 2;
		while (thistick == HAL_GetTick());
	}

	return (set_rf_level(level));
}


int cmd_fmtone (cmd_param_t** params, fifo_t* in __attribute__((unused)), fifo_t* out __attribute__((unused))) {
	int ms;
	int dev;

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_NUM) {
		return 0;
	}
	dev = (*params)->n;
	cmd_param_consume(params);

	if (dev < 10 || dev > 1000) { // 10 kHz - 1 MHz
		console_printf(invalid_val, dev);
		return 0;
	}

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_NUM) {
		return 0;
	}
	ms = (*params)->n;
	cmd_param_consume(params);

	if (ms < 0 || ms > 3600000) { // max 1 hour
		console_printf(invalid_val, ms);
		return 0;
	}

	uint32_t tickstart = HAL_GetTick();
	uint32_t thistick;
	int freq = frequency_getter(NULL);
	int state = 0;

	while (((thistick = HAL_GetTick()) - tickstart) < ms) {
		if (switchbreak()) {
			console_printf("Break");
			break;
		}
		if (!set_rf_frequency(freq + (state ? dev : -dev))) {
			break;
		}
		state += 1; state %= 2;
		while (thistick == HAL_GetTick());
	}

	return (set_rf_frequency(freq));
}




int cmd_rfon (cmd_param_t** params __attribute__((unused)), fifo_t* in __attribute__((unused)), fifo_t* out __attribute__((unused))) {
	set_rf_output(1);
	print_cfg();
	return 1;
}


int cmd_rfoff (cmd_param_t** params __attribute__((unused)), fifo_t* in __attribute__((unused)), fifo_t* out __attribute__((unused))) {
	set_rf_output(0);
	print_cfg();
	return 1;
}


int setup_persona_commands (void) {

	keyword_add("malloctest", "", cmd_malloctest);
	keyword_add("sleep", "[millisecs] - sleep", cmd_sleep);
	keyword_add("format", "- format EEPROM", cmd_format);

	// RF GENERATOR ==================================
	keyword_add("rfoff", "- RF off", cmd_rfoff);
	keyword_add("rfon", "- RF on", cmd_rfon);
	keyword_add("fmtone", " [dev] [ms] - FM tone", cmd_fmtone);
	keyword_add("amtone", " [ms] - AM tone", cmd_amtone);

	return 0;
}
