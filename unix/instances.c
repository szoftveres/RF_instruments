#include "adcdac.h"
#include "instances.h"
#include "ofdmmodem.h"
#include "config_def.h"
#include "os/globals.h"  // config instance
#include "os/keyword.h"  // cmd_params_t
#include <string.h> // memcpy
#include <stdio.h> // EOF
#include "os/hal_plat.h" // malloc free

max2871_t* rf_pll;

bda4700_t *attenuator;

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

int cmd_dacsink (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	return dacsink_setup(in);
}


int cmd_adcsrc (cmd_param_t** params, fifo_t* in, fifo_t* out) {
    int fs;
    if (get_cmd_arg_type(params) != CMD_ARG_TYPE_NUM) {
        console_printf("fs needed");
        return 0;
    }
    fs = (*params)->n;
    cmd_param_consume(params);

    return adcsrc_setup(out, fs);
}


#define OFDM_FS (20000)


int cmd_ofdm_tx (cmd_param_t** params, fifo_t* in, fifo_t* out) {
    int fs = OFDM_FS;
    ofdm_pkt_t p;

    char* data;

    start_audio_out(fs);

    data = "Kellemes es Boldog Karacsonyt kivan onnek a Vodafone";
    ofdm_packetize(&p, data, strlen(data)+1);
    ofdm_txpkt(fs, &p);

    for (int i = 0; i != OFDM_FS/4; i++) {
        play_int16_sample(0);
    }

    data = "Es sok boldogsagot az ujevhez";
    ofdm_packetize(&p, data, strlen(data)+1);
    ofdm_txpkt(fs, &p);


    stop_audio_out();
    return 1;
}


int cmd_ofdm_rx (cmd_param_t** params, fifo_t* in, fifo_t* out) {
    int fs = OFDM_FS;
    ofdm_pkt_t p;
    char* data;
    int ampl;

    memset(&p, 0x00, sizeof(ofdm_pkt_t));
    start_audio_in(fs);
    while (ofdm_rxpkt(fs, &p, &ampl) < 0);
    stop_audio_in();

    console_printf_e("(ampl:%i)", ampl);
    if (ofdm_depacketize(&p, &data) >= 0) {
        console_printf("[%s]", data);
    }

    return 1;
}



int setup_persona_commands (void) {

	keyword_add("rx", "- test", cmd_ofdm_rx);
	keyword_add("tx", "- test", cmd_ofdm_tx);
	keyword_add("dacsnk", "->DAC", cmd_dacsink);
	keyword_add("adcsrc", "ADC [fs]->", cmd_adcsrc);

	return 0;
}

