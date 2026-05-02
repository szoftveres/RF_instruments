#include "main.h" // PIN names
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

static const char* invalid_val = "Invalid value \'%i\'\n";


double set_rf_frequency (uint32_t khz) {
	double actual_kHz;

	get_cal_range_index((int)khz);
	actual_kHz = max2871_freq(rf_pll, khz);
	while (!HAL_GPIO_ReadPin(PLL1_LOCK_DETECT_GPIO_Port, PLL1_LOCK_DETECT_Pin)); // Wait for LD
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
	printf_f(STDERR, "RF: %i kHz, %i dBm, output %s\n", config.fields.khz, config.fields.level, config.fields.rfon ? "on" : "off");
}


/*-----------------------------------------*/

#include <stdlib.h>


int freq_func (cmd_context_s* ctxt) {
	int rc = 1;

	if (get_data_obj_type(ctxt->params) == OBJ_TYPE_NUM) {
		rc = set_rf_frequency(ctxt->params->n) ? 1 : 0;
		obj_consume(&(ctxt->params));
		print_cfg();
	} else {
		obj_add_num(&(ctxt->ret), config.fields.khz);
	}
	return rc;
}



int rflevel_func (cmd_context_s* ctxt) {
	int rc = 1;

	if (get_data_obj_type(ctxt->params) == OBJ_TYPE_NUM) {
		rc = set_rf_level(ctxt->params->n);
		obj_consume(&(ctxt->params));
		print_cfg();
	} else {
		obj_add_num(&(ctxt->ret), config.fields.level);
	}
	return rc;
}


int cmd_malloctest (cmd_context_s* ctxt) {
	int i = 0;
	const int size = 1024;
	while (malloc(size)) {
		i++;
	}
	printf_f(STDOUT, "size:%i, total:%i\n", size, size*i);
	cpu_halt();
	return 1;
}


#include "../os/fatsmall_fs.h"

int cmd_format (cmd_context_s* ctxt) {
	printf_f(STDERR, " type \"yes\"> ");
	/*
	char* line = online_reader->getline(online_reader);
	if (strcmp(line, "yes")) {
		printf_f(STDERR, "aborted\n");
		return 1;
	}
	fs_format(eepromfs, 16);
	*/
	return cmd_fsinfo();
}


int cmd_sleep (cmd_context_s* ctxt) {
	int ms;

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		return 0;
	}
	ms = ctxt->params->n;
	obj_consume(&(ctxt->params));

	if (ms < 0 || ms > 3600000) { // max 1 hour
		printf_f(STDERR, invalid_val, ms);
		return 0;
	}

	uint32_t tickstart = HAL_GetTick();

	while((HAL_GetTick() - tickstart) < ms) {
		if (switchbreak()) {
			printf_f(STDERR, "Break\n");
			break;
		}
	}
	return 1;
}


int cmd_amtone (cmd_context_s* ctxt) {
	int ms;

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		return 0;
	}
	ms = ctxt->params->n;
	obj_consume(&(ctxt->params));

	if (ms < 0 || ms > 3600000) { // max 1 hour
		printf_f(STDERR, invalid_val, ms);
		return 0;
	}

	uint32_t tickstart = HAL_GetTick();
	uint32_t thistick;
	int level = config.fields.level;
	int state = 0;

	while (((thistick = HAL_GetTick()) - tickstart) < ms) {
		if (switchbreak()) {
			printf_f(STDERR, "Break\n");
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


int cmd_fmtone (cmd_context_s* ctxt) {
	int ms;
	int dev;

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		return 0;
	}
	dev = ctxt->params->n;
	obj_consume(&(ctxt->params));

	if (dev < 1 || dev > 1000) { // 10 kHz - 1 MHz
		printf_f(STDERR, invalid_val, dev);
		return 0;
	}

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		return 0;
	}
	ms = ctxt->params->n;
	obj_consume(&(ctxt->params));

	if (ms < 0 || ms > 3600000) { // max 1 hour
		printf_f(STDERR, invalid_val, ms);
		return 0;
	}

	uint32_t tickstart = HAL_GetTick();
	uint32_t thistick;
	int freq = config.fields.khz;
	int state = 0;

	while (((thistick = HAL_GetTick()) - tickstart) < ms) {
		if (switchbreak()) {
			printf_f(STDERR, "Break\n");
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


int cmd_rfon (cmd_context_s* ctxt) {
	set_rf_output(1);
	print_cfg();
	return 1;
}


int cmd_rfoff (cmd_context_s* ctxt) {
	set_rf_output(0);
	print_cfg();
	return 1;
}


int cmd_instctl_test (cmd_context_s* ctxt) {
	int n;

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		return 0;
	}
	n = ctxt->params->n;
	obj_consume(&(ctxt->params));

	console_send_u32(0xB43355AA);

	console_send_i32(n);

	return 1;
}


int setup_persona_commands (void) {
	keyword_add("instctltest", "- test", cmd_instctl_test);
	keyword_add("malloctest", "", cmd_malloctest);
	keyword_add("sleep", "[millisecs] - sleep", cmd_sleep);
	keyword_add("format", "- format EEPROM", cmd_format);

	// RF GENERATOR ==================================
	keyword_add("rfoff", "- RF off", cmd_rfoff);
	keyword_add("rfon", "- RF on", cmd_rfon);
	keyword_add("fmtone", " [dev] [ms] - FM tone", cmd_fmtone);
	keyword_add("amtone", " [ms] - AM tone", cmd_amtone);
	keyword_add("level", "(dBm)", rflevel_func);
	keyword_add("freq", "(kHz)", freq_func);

	return 0;
}
