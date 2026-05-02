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
#include "../os/nmea0183.h"

max2871_t* rf_pll;
max2871_t* lo_pll;
bda4700_t *attenuator;

fs_t *eepromfs; // FORMAT

static const char* invalid_val = "Invalid value \'%i\'\n";
static const char* name_expected = "\"name\" expected\n";



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
	printf_f(STDERR, "RF: %i kHz, %i dBm, output %s\n", config.fields.khz, config.fields.level, config.fields.rfon ? "on" : "off");
}


int baud_to_samples (int baud) {
	return config.fields.fs / baud;
}


/*-----------------------------------------*/

#include <stdlib.h>

int cmd_asel (cmd_context_s* ctxt) {
	int n;

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		return 0;
	}
	n = ctxt->params->n;
	obj_consume(&(ctxt->params));
	HAL_GPIO_WritePin(CHANNEL_SELECT_GPIO_Port, CHANNEL_SELECT_Pin, n ? GPIO_PIN_SET : GPIO_PIN_RESET); // 1: reflected, 0: through

	return 1;
}


int rflevel_func (cmd_context_s* ctxt) {
	int rc = 1;

	if (get_data_obj_type(ctxt->params) == OBJ_TYPE_NUM) {
		rc = set_rf_level(ctxt->params->n);
		obj_consume(&(ctxt->params));
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


int cmd_dacsink (cmd_context_s* ctxt) {
	return dacsink_setup(ctxt->in);
}


int cmd_adcsrc (cmd_context_s* ctxt) {
	int fs;

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		return 0;
	}
	fs = ctxt->params->n;
	obj_consume(&(ctxt->params));

	return adcsrc_setup(ctxt->out, fs);
}


int cmd_ofdm_tx (cmd_context_s* ctxt) {
    ofdm_pkt_t p;

	if (get_data_obj_type(ctxt->params) == OBJ_TYPE_STR) {
		ofdm_packetize(&p, ctxt->params->str, strlen(ctxt->params->str)+1);
		obj_consume(&(ctxt->params));
		ofdm_txpkt(&p);
		return 1;
	}

	int seq = 0;
	char msg[64];

    while (!switchbreak()) {
    	sprintf(msg, "%04i OFDM message, very long message", seq);
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
        	printf_f(STDOUT, "%s, level:%i\n", data, level);
            break;
        }
    }
    ledoff();
    return 1;
}


int cmd_bpsk_tx (cmd_context_s* ctxt) {
    bpsk_pkt_t p;

	if (get_data_obj_type(ctxt->params) == OBJ_TYPE_STR) {
		bpsk_packetize(&p, ctxt->params->str, strlen(ctxt->params->str)+1);
		obj_consume(&(ctxt->params));
		bpsk_txpkt(&p);
		return 1;
	}

	int seq = 0;
	char msg[64];

    while (!switchbreak()) {
    	sprintf(msg, "%04i BPSK message", seq);
    	bpsk_packetize(&p, msg, strlen(msg)+1);
    	bpsk_txpkt(&p);
        //delay_ms(2000);
        seq += 1;
    }

    return 1;
}


int cmd_bpsk_rx (cmd_context_s* ctxt) {
    bpsk_pkt_t p;
    char* data;
    int level;

    while (!switchbreak()) {
        memset(&p, 0x00, sizeof(ofdm_pkt_t));
        if (bpsk_rxpkt(&p, &level) < 0) {
            continue;
        }
        if (bpsk_depacketize(&p, &data) >= 0) {
        	printf_f(STDOUT, "%s, level:%i\n", data, level);
            break;
        }
    }

    return 1;
}


typedef struct gps_context_s {
	int in_fd;
} gps_context_t;

char get_gps_char (nmea0183_t* this) {
	char buf;
	gps_context_t *ctxt = (gps_context_t*)this->ctxt;
	read_f_all(fs, ctxt->in_fd, &buf, 1);
	return buf;
}

int cmd_gps (cmd_context_s* ctxt) {
	char msg[64];
	gps_context_t gps_ctxt;
	int fdin;
	int fdout;
	int len;

    if (get_data_obj_type(ctxt->params) != OBJ_TYPE_STR) {
    	printf_f(STDERR, name_expected);
		return 0;
	}

	fdin = open_f(fs, ctxt->params->str, FS_O_READONLY);
	obj_consume(&(ctxt->params));
	if (fdin < 0) {
		printf_f(STDERR, "open fail\n");
		return 0;
	}
	gps_ctxt.in_fd = fdin;

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_STR) {
		printf_f(STDERR, name_expected);
		close_f(fs, fdin);
		return 0;
	}

	fdout = open_f(fs, ctxt->params->str, FS_O_CREAT | FS_O_TRUNC);
	obj_consume(&(ctxt->params));

	if (fdout < 0) {
		printf_f(STDERR, "dst open fail\n");
		close_f(fs, fdin);
		return 0;
	}

	nmea0183_t *gps = nmea0183_create (get_gps_char, &gps_ctxt);

	nmea0183_update(gps);
	len = sprintf(msg, "%i.%i,%i.%i,%i:%02i:%02i\n", gps->lat_i, gps->lat_f, gps->lon_i, gps->lon_f, gps->hour, gps->min, gps->sec);
	write_f_all(fs, fdout, msg, len);

	nmea0183_destroy(gps);
	close_f(fs, fdin);
	close_f(fs, fdout);

	return 1;
}


int cmd_instctl_test (cmd_context_s* ctxt) {
	int n;

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		return 0;
	}
	n = ctxt->params->n;
	obj_consume(&(ctxt->params));

	// Sync word
	console_send_u32(0xB43355AA);

	// Data
	console_send_i32(n);

	return 1;
}


int cmd_vna (cmd_context_s* ctxt) {

	int freq;
	int lck = 0;
	rfport_rx_t rfmeas;

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		printf_f(STDERR, "Freq needed\n");
		return 0;
	}
	freq = ctxt->params->n;
	obj_consume(&(ctxt->params));

	max2871_freq(rf_pll, (double)freq);
	max2871_freq(lo_pll, (double)(freq + 10));

	/* Syncword */
	console_send_u32(0xB43355AA); // gives some time for the PLLs to stabilize

	while (!HAL_GPIO_ReadPin(MISO_INPUT_GPIO_Port, MISO_INPUT_Pin)) {
		HAL_Delay(10);
		lck += 1;
		if (lck > 20) {
			printf_f(STDERR, "PLL lock error\n");
			return 0;
		}
	}
	rfport_rx_meas(10000, 800, &rfmeas);

	/* Reference */
	console_send_i32(rfmeas.ref_i);
	console_send_i32(rfmeas.ref_q);

	/* Reference absolute amplitude */
	console_send_i32(rfmeas.ref_ampl);

	/* Receiver */
	console_send_i32(rfmeas.meas_i);
	console_send_i32(rfmeas.meas_q);

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

	keyword_add("asel", "0/1", cmd_asel);
	keyword_add("level", "(dBm)", rflevel_func);

	keyword_add("rfoff", "- RF off", cmd_rfoff);
	keyword_add("rfon", "- RF on", cmd_rfon);

	keyword_add("vna", "- test", cmd_vna);

	keyword_add("instctltest", "- test", cmd_instctl_test);

	keyword_add("gps", "\"src\" \"dst\"", cmd_gps);

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

