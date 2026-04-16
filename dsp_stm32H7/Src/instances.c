#include <adcdac.h>
#include "main.h" // GPIO custom names
#include "instances.h"
#include "config_def.h"
#include "../os/globals.h"  // config instance
#include <string.h> // memcpy
#include <stdio.h> // EOF
#include "../os/hal_plat.h" // malloc free
#include "../os/keyword.h" // command structure
#include "../os/resource.h"
#include "../os/modem.h"
#include "stm32h7xx_hal.h"  // HAL get tick
#include "rfport_rx.h"


max2871_t* rf_pll;
max2871_t* lo_pll;
bda4700_t *attenuator;

fs_t *eepromfs; // FORMAT

nmea0183_t* gps;

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
	max2871_rfa_power(rf_pll, -1);
	max2871_rfa_power(lo_pll, -1);

	max2871_rfa_out(rf_pll, on);
	max2871_rfa_out(lo_pll, on);

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


int set_fs (int fs) {
	config.fields.fs = fs;
	return 1;
}


int set_fc (int fc) {
	config.fields.fc = fc;
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
	set_fs(config.fields.fs);
	set_fc(config.fields.fc);
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



int fs_setter (void * context, int fs) {
	if (!set_fs(fs)) {
		console_printf(invalid_val, fs);
		return 0;
	}
	console_printf("fs: %i Hz", fs);
	return 1;
}

int fs_getter (void * context) {
	return config.fields.fs;
}

int fc_setter (void * context, int fc) {
	if (!set_fc(fc)) {
		console_printf(invalid_val, fc);
		return 0;
	}
	console_printf("fc: %i Hz", fc);
	return 1;
}

int fc_getter (void * context) {
	return config.fields.fc;
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

int baud_to_samples (int baud) {
	return config.fields.fs / baud;
}


int asel_setter (void * context, int value) {
	resource_t* resource = (resource_t*)context;
	resource->value = value;
	HAL_GPIO_WritePin(CHANNEL_SELECT_GPIO_Port, CHANNEL_SELECT_Pin, value ? GPIO_PIN_SET : GPIO_PIN_RESET); // 1: reflected, 0: through
	return 1;
}


int asel_getter (void * context) {
	resource_t* resource = (resource_t*)context;
	return resource->value;
}



/*-----------------------------------------*/

#include <stdlib.h>

int cmd_malloctest (cmd_context_s* ctxt) {
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
int cmd_format (cmd_context_s* ctxt) {
	char* line = terminal_get_line(online_input, " type \"yes\"> ", 1);
	if (strcmp(line, "yes")) {
		console_printf("aborted");
		return 1;
	}
	fs_format(eepromfs, 16);
	return cmd_fsinfo();
}


int cmd_sleep (cmd_context_s* ctxt) {
	int ms;

	if (get_cmd_arg_type(ctxt->params) != CMD_ARG_TYPE_NUM) {
		return 0;
	}
	ms = ctxt->params->n;
	cmd_param_consume(&(ctxt->params));

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


int cmd_dacsink (cmd_context_s* ctxt) {
	return dacsink_setup(ctxt->in);
}


int cmd_adcsrc (cmd_context_s* ctxt) {
	int fs;

	if (get_cmd_arg_type(ctxt->params) != CMD_ARG_TYPE_NUM) {
		return 0;
	}
	fs = ctxt->params->n;
	cmd_param_consume(&(ctxt->params));

	return adcsrc_setup(ctxt->out, fs);
}


int cmd_ofdm_tx (cmd_context_s* ctxt) {
    ofdm_pkt_t p;

	if (get_cmd_arg_type(ctxt->params) == CMD_ARG_TYPE_STR) {
		ofdm_packetize(&p, ctxt->params->str, strlen(ctxt->params->str)+1);
		cmd_param_consume(&(ctxt->params));
		ofdm_txpkt(&p);
		return 1;
	}

	int seq = 0;
	char msg[64];

    while (!switchbreak()) {
    	sprintf(msg, "%04i OFDM message", seq);
        ofdm_packetize(&p, msg, strlen(msg)+1);
        ofdm_txpkt(&p);
        //delay_ms(2000);
        seq += 1;
    }
    return 1;
}


int cmd_ofdm_rx (cmd_context_s* ctxt) {
    ofdm_pkt_t p;
    char* data;
    ledon();
    int level;
    while (!switchbreak()) {
        memset(&p, 0x00, sizeof(ofdm_pkt_t));
        if (ofdm_rxpkt(&p, &level) < 0) {
            continue;
        }
        if (ofdm_depacketize(&p, &data) >= 0) {
            console_printf("%s, level:%i", data, level);
            break;
        }
    }
    ledoff();
    return 1;
}


int cmd_bpsk_tx (cmd_context_s* ctxt) {
    bpsk_pkt_t p;

	if (get_cmd_arg_type(ctxt->params) == CMD_ARG_TYPE_STR) {
		bpsk_packetize(&p, ctxt->params->str, strlen(ctxt->params->str)+1);
		cmd_param_consume(&(ctxt->params));
		bpsk_txpkt(&p);
		return 1;
	}

	int seq = 0;
	char msg[64];

    while (!switchbreak()) {
    	sprintf(msg, "%04i BPSK message", seq);
    	bpsk_packetize(&p, msg, strlen(msg)+1);
    	bpsk_txpkt(&p);
        delay_ms(2000);
        seq += 1;
    }

    return 1;
}


int cmd_bpsk_rx (cmd_context_s* ctxt) {
    bpsk_pkt_t p;
    char* data;

    while (!switchbreak()) {
        memset(&p, 0x00, sizeof(ofdm_pkt_t));
        if (bpsk_rxpkt(&p) < 0) {
            continue;
        }
        if (bpsk_depacketize(&p, &data) >= 0) {
            console_printf("[%s]", data);
        }
    }

    return 1;
}


int cmd_gps (cmd_context_s* ctxt) {
	char msg[64];
	ofdm_pkt_t p;

	nmea0183_update(gps);
	sprintf(msg, "%i.%i,%i.%i,%i:%02i:%02i", gps->lat_i, gps->lat_f, gps->lon_i, gps->lon_f, gps->hour, gps->min, gps->sec);
	console_printf(msg);
	ofdm_packetize(&p, msg, strlen(msg)+1);
	ofdm_txpkt(&p);
	return 1;
}


int cmd_instctl_test (cmd_context_s* ctxt) {
	int n;

	if (get_cmd_arg_type(ctxt->params) != CMD_ARG_TYPE_NUM) {
		return 0;
	}
	n = ctxt->params->n;
	cmd_param_consume(&(ctxt->params));

	// Sync word
	console_send_u32(0xB43355AA);

	// Data
	console_send_i32(n);

	return 1;
}


void cmd_rfport_meas (void) {
	rfport_rx_t rfmeas;

	rfport_rx_meas(10000, 800, &rfmeas);

	/* Syncword */
	console_send_u32(0xB43355AA);

	/* Ref */
	console_send_i32(rfmeas.ref_i);
	console_send_i32(rfmeas.ref_q);

	/* Ref ampl */
	console_send_i32(rfmeas.ref_ampl);

	/* Port 1 */
	console_send_i32(rfmeas.meas_i);
	console_send_i32(rfmeas.meas_q);
}


int cmd_vna (cmd_context_s* ctxt) {

	int freq;

	if (get_cmd_arg_type(ctxt->params) != CMD_ARG_TYPE_NUM) {
		console_printf("Freq needed");
		return 0;
	}
	freq = ctxt->params->n;
	cmd_param_consume(&(ctxt->params));

	max2871_freq(rf_pll, (double)freq);
	max2871_freq(lo_pll, (double)(freq + 10));

	while (!HAL_GPIO_ReadPin(MISO_INPUT_GPIO_Port, MISO_INPUT_Pin)) {
		console_printf("waiting for lock");
		HAL_Delay(500);
	}

	cmd_rfport_meas();
	return 1;
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


int setup_persona_commands (void) {

	resource_add("asel", NULL, asel_setter, asel_getter);
	resource_add("level", NULL, rflevel_setter, rflevel_getter);

	keyword_add("rfoff", "- RF off", cmd_rfoff);
	keyword_add("rfon", "- RF on", cmd_rfon);

	keyword_add("vna", "- test", cmd_vna);

	keyword_add("instctltest", "- test", cmd_instctl_test);

	keyword_add("gps", "- test", cmd_gps);

	keyword_add("otx", "- test", cmd_ofdm_tx);
	keyword_add("orx", "- test", cmd_ofdm_rx);
	keyword_add("btx", "- test", cmd_bpsk_tx);
	keyword_add("brx", "- test", cmd_bpsk_rx);

	keyword_add("malloctest", "", cmd_malloctest);

	keyword_add("dacsnk", "->DAC", cmd_dacsink);
	keyword_add("adcsrc", "[fs] ADC->", cmd_adcsrc);

	keyword_add("sleep", "[millisecs] - sleep", cmd_sleep);
	keyword_add("format", "- format EEPROM", cmd_format);


	return 0;
}

