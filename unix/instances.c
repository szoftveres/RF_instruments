#include "adcdac.h"
#include "instances.h"
#include "os/modem.h"
#include "config_def.h"
#include "os/globals.h"  // config instance
#include "os/keyword.h"  // cmd_params_t
#include <string.h> // memcpy
#include <stdio.h> // EOF
#include "os/hal_plat.h" // malloc free

static const char* invalid_val = "Invalid value \'%i\'\n";




void global_cfg_override (void) {
}

void apply_cfg (void) {
}


void cfg_override (void) {
	config.fields.rfon = 1; // always load with RF on
}



void print_cfg (void) {
	printf_f(STDERR, "RF: %i kHz, %i dBm, output %s\n", config.fields.khz, config.fields.level, config.fields.rfon ? "on" : "off");
}


/*
int fs_setter (void * context, int fs) {
	if (!set_fs(fs)) {
		printf_f(STDERR, invalid_val, fs);
		return 0;
	}
	printf_f(STDERR, "fs: %i Hz\n", fs);
	return 1;
}

int fc_setter (void * context, int fc) {
	if (!set_fc(fc)) {
		printf_f(STDERR, invalid_val, fc);
		return 0;
	}
	printf_f(STDERR, "fc: %i Hz\n", fc);
	return 1;
}

int dac1_setter (void * context, int aval) {
	resource_t* resource = (resource_t*)context;
	if (aval < 0 || aval >= dac_max()) {
		printf_f(STDERR, invalid_val, aval);
		return 0;
	}
	resource->value = aval;
	dac1_outv((uint32_t)aval);
	return 1;
}
*/

int cmd_dacsink (cmd_context_s* ctxt) {
	return dacsink_setup(ctxt->in);
}


int cmd_adcsrc (cmd_context_s* ctxt) {
    int fs;
    if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
        printf_f(STDERR, "fs needed\n");
        return 0;
    }
    fs = ctxt->params->n;
    obj_consume(&(ctxt->params));

    return adcsrc_setup(ctxt->out, fs);
}



int cmd_ofdm_tx (cmd_context_s* ctxt) {
    ofdm_pkt_t p;

    char* data[] = {"Kellemes es Boldog Karacsonyt kivan onnek a Vodafone",
                    "Best CRC Polynomials are not always the best",
                    "Here at Hackaday we love floppy disks.",
                    "====----====----====----====----"

                    };

    while (1) {
        int n = 0;
        for (int i = 0; i != 4; i++) {
            ofdm_packetize(&p, data[i], strlen(data[i])+1);
            n += strlen(data[i])+1;
            ofdm_txpkt(&p);
        }
        printf_f(STDERR, "%i bytes\n", n);
    }
    return 1;
}


int cmd_ofdm_rx (cmd_context_s* ctxt) {
    ofdm_pkt_t p;
    int level;
    char* data;

    while (1) {
        memset(&p, 0x00, sizeof(ofdm_pkt_t));
        if (ofdm_rxpkt(&p, &level) < 0) {
            continue;
        }
        if (ofdm_depacketize(&p, &data) >= 0) {
        	printf_f(STDOUT, "%s, level:%i\n", data, level);
            break;
        }
    }

    return 1;
}



int cmd_bpsk_tx (cmd_context_s* ctxt) {
    bpsk_pkt_t p;

    char* data[] = {"Kellemes es Boldog Karacsonyt kivan onnek a Vodafone",
                    "Best CRC Polynomials are not always the best",
                    "Here at Hackaday we love floppy disks.",
                    "====----====----====----====----"

                    };

    while (1) {
        int n = 0;
        for (int i = 0; i != 4; i++) {
            bpsk_packetize(&p, data[i], strlen(data[i])+1);
            n += strlen(data[i])+1;
            bpsk_txpkt(&p);
        }
        printf_f(STDERR, "%i bytes\n", n);
    }

    return 1;
}

int cmd_bpsk_rx (cmd_context_s* ctxt) {
    bpsk_pkt_t p;
    char* data;
    int level;

    while (1) {
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




int setup_persona_commands (void) {

	keyword_add("brx", "- test", cmd_bpsk_rx);
	keyword_add("btx", "- test", cmd_bpsk_tx);
	keyword_add("orx", "- test", cmd_ofdm_rx);
	keyword_add("otx", "- test", cmd_ofdm_tx);
	keyword_add("dacsnk", "->DAC", cmd_dacsink);
	keyword_add("adcsrc", "ADC [fs]->", cmd_adcsrc);

	return 0;
}

