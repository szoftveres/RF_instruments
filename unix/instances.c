#include "adcdac.h"
#include "instances.h"
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


#define OFDM_PKT_PAYLOAD_MAX (32)
#define OFDM_ANTIPREAMBLE (0x55)

typedef struct __attribute__((packed)) {
    uint8_t antipreamble; // To create a sharp boundary
    uint8_t len;
    uint8_t crc;
} ofdm_pkt_header_t;


typedef union __attribute__((packed)) ofdm_packet_u {
    uint8_t byte[0];
    struct __attribute__((packed)) {
        ofdm_pkt_header_t h;
        char payload[OFDM_PKT_PAYLOAD_MAX];
    };
} ofdm_pkt_t;

#define OFDM_PKT_TOTAL_LEN(l) (((int)(l)) + ((int)sizeof(ofdm_pkt_header_t)))

int ofdm_packetize (ofdm_pkt_t* p, void *data, int len) {
    if (len > OFDM_PKT_PAYLOAD_MAX) {
        return 0;
    }
    p->h.len = (uint8_t)len;
    p->h.crc = 0;
    p->h.antipreamble = OFDM_ANTIPREAMBLE;
    if (len) {
        memcpy(p->payload, data, len);
    }
    p->h.crc = crc8(p->byte, OFDM_PKT_TOTAL_LEN(p->h.len));
    return len;
}

int ofdm_depacketize (ofdm_pkt_t* p, void **data) {
    uint8_t crc = 0;
    if (p->h.len > OFDM_PKT_PAYLOAD_MAX) {
        return -1;
    }
    if (data) {
        *data = p->payload;
    }
    crc = p->h.crc;
    p->h.crc = 0;
    if (crc8(p->byte, OFDM_PKT_TOTAL_LEN(p->h.len)) != crc) {
        return -1;
    }
    return p->h.len;
}



#define OFDM_MODEM_OVERSAMPLE_RATE (5)
#define OFDM_MODEM_SPS (125)
#define OFDM_FS (20000)
#define OFDM_CENTER_CARRIER (2000)


int ofdm_txpkt (int fs, ofdm_pkt_t *p);
int ofdm_rxpkt (int fs, ofdm_pkt_t *p);

int cmd_ofdm_tx (cmd_param_t** params, fifo_t* in, fifo_t* out) {
    int fs = OFDM_FS;
    ofdm_pkt_t p;

    char* data;

    start_audio_out(fs);


    data = "Boldog Karacsonyt kivan";
    ofdm_packetize(&p, data, strlen(data)+1);
    ofdm_txpkt(fs, &p);

    data = " onnek a Vodafone";
    ofdm_packetize(&p, data, strlen(data)+1);
    ofdm_txpkt(fs, &p);


    stop_audio_out();
    return 1;
}


int cmd_ofdm_rx (cmd_param_t** params, fifo_t* in, fifo_t* out) {
    int fs = OFDM_FS;
    ofdm_pkt_t p;
    char* data;

    start_audio_in(fs);
    ofdm_rxpkt(fs, &p);
    stop_audio_in();

    if (ofdm_depacketize(&p, &data) >= 0) {
        console_printf("[%s]", data);
    }

    return 1;
}



int ofdm_txpkt (int fs, ofdm_pkt_t *p) {
	int fft_len = OFDM_CARRIER_PAIRS * 2 * OFDM_MODEM_OVERSAMPLE_RATE; // carrier pairs * Nyquist * Oversample rate
    int target_fs = fft_len * OFDM_MODEM_SPS;
    int dec = fs / target_fs;
    int training = 3;
    int symbolampl = ofdm_cplx_u8_symbolampl(fft_len, 32768);

    dds_t *mixer = dds_create(fs, OFDM_CENTER_CARRIER);


	int *i_symbol = (int*)t_malloc(fft_len * sizeof(int));
	int *q_symbol = (int*)t_malloc(fft_len * sizeof(int));

	int *i_baseband = (int*)t_malloc(fft_len * sizeof(int));
	int *q_baseband = (int*)t_malloc(fft_len * sizeof(int));

	int wave;
    int16_t sample_out;
    int i_a;
    int q_a;

    // Sending training symbols
    for (int t = 0; t != training; t++) {
        // training symbol
        memset(i_symbol, 0x00, fft_len * sizeof(int));
        memset(q_symbol, 0x00, fft_len * sizeof(int));
        for (int n = -2; n != 3; n++) {
            i_symbol[ofdm_carrier_to_idx(n, fft_len)] = symbolampl;
            q_symbol[ofdm_carrier_to_idx(n, fft_len)] = 0;
        }
        ofdm_cplx_encode_symbol(i_symbol, q_symbol, i_baseband, q_baseband, fft_len); // Training
        for (int f = 0; f != fft_len; f += 1) {
            for (int d = 0; d != dec; d += 1) {
                dds_next_sample(mixer, &i_a, &q_a);
                wave = ((i_baseband[f] * i_a) + (q_baseband[f] * q_a)) / magnitude_const();
                sample_out = (int16_t)wave;
                play_int16_sample(&sample_out);
            }
        }
    }

    int pp = 0;

    while (pp != OFDM_PKT_TOTAL_LEN(p->h.len)) {
        memset(i_symbol, 0x00, fft_len * sizeof(int));
        memset(q_symbol, 0x00, fft_len * sizeof(int));

        ofdm_u8_to_symbol(p->byte[pp], 0, 0, i_symbol, q_symbol, fft_len, symbolampl);
        ofdm_cplx_encode_symbol(i_symbol, q_symbol, i_baseband, q_baseband, fft_len); // Training
        for (int f = 0; f != fft_len; f += 1) {
            for (int d = 0; d != dec; d++) {
                dds_next_sample(mixer, &i_a, &q_a);
                wave = ((i_baseband[f] * i_a) + (q_baseband[f] * q_a)) / magnitude_const();
                sample_out = (int16_t)wave;
                play_int16_sample(&sample_out);
            }
        }

        pp += 1;
    }


    // Negative trailing symbol
    for (int f = 0; f != fft_len; f += 1) {
        for (int d = 0; d != dec; d++) {
            dds_next_sample(mixer, &i_a, &q_a);
            wave = ((-i_baseband[f] * i_a) + (-q_baseband[f] * q_a)) / magnitude_const();
            sample_out = (int16_t)wave;
            play_int16_sample(&sample_out);
        }
    }

	t_free(i_symbol);
	t_free(q_symbol);
	t_free(i_baseband);
	t_free(q_baseband);
    dds_destroy(mixer);

	return 1;
}



int ofdm_rxpkt (int fs, ofdm_pkt_t *p) {

	int fft_len = OFDM_CARRIER_PAIRS * 2 * OFDM_MODEM_OVERSAMPLE_RATE; // carrier pairs * Nyquist * Oversample rate
    int target_fs = fft_len * OFDM_MODEM_SPS;
    int dec = fs / target_fs;
    int symbolampl = ofdm_cplx_u8_symbolampl(fft_len, 32768);

    dds_t *mixer = dds_create(fs, OFDM_CENTER_CARRIER);


	int *i_symbol = (int*)t_malloc(fft_len * sizeof(int));
	int *q_symbol = (int*)t_malloc(fft_len * sizeof(int));
	int *i_eq = (int*)t_malloc(fft_len * sizeof(int));
	int *q_eq = (int*)t_malloc(fft_len * sizeof(int));
	int *avg = (int*)t_malloc(fft_len * sizeof(int));

	int *i_baseband = (int*)t_malloc(fft_len * sizeof(int));
	int *q_baseband = (int*)t_malloc(fft_len * sizeof(int));

	int16_t sample_in;
	int wave;
    int running = 1;
    int i_a;
    int q_a;
    int pp;

    p->h.len = OFDM_PKT_PAYLOAD_MAX;

    memset(i_symbol, 0x00, fft_len * sizeof(int));
    memset(q_symbol, 0x00, fft_len * sizeof(int));
    memset(i_eq, 0x00, fft_len * sizeof(int));
    memset(q_eq, 0x00, fft_len * sizeof(int));
    memset(avg, 0x00, fft_len * sizeof(int));


    // Setting up the decimating filter
    int taps = fir_ntaps(dec, 2);
    int *tap = fir_create_taps(dec, 2);
    int norm = fir_normf(tap, taps);
    int *buf_i = (int*)t_malloc(taps * sizeof(int));
	int *buf_q = (int*)t_malloc(taps * sizeof(int));
    int bp = 0;
    int wp = 0;
    int statemachine = 0;
    int prevpeak = 0;
    int threshold = symbolampl * symbolampl ;

    while (running) {
        int acc;

        dds_next_sample(mixer, &i_a, &q_a);
        record_int16_sample(&sample_in); wave = (int)sample_in;

	    buf_i[bp] = wave * i_a / magnitude_const();
	    buf_q[bp] = wave * q_a / magnitude_const();
        bp++;
	    if (bp < taps) {
		    continue;
	    }
	    bp -= dec;  // due to memmove in fir_work
        i_a = fir_work(buf_i, tap, taps, dec) / norm;
	    q_a = fir_work(buf_q, tap, taps, dec) / norm;

        switch (statemachine) {
        case 0:     // Preamble detection, by autocorrelating the
                    // first two (identical) preamble symbols
                    // https://www.youtube.com/watch?v=ysgONmM6iKk
            int i_b = i_a;
            int q_b = q_a;
            cplx_mul(&i_a, &q_a, i_eq[0], -q_eq[0], symbolampl);
            avg[wp] = i_a;

            acc = 0;
            for (int n = 0; n != fft_len; n++) {
                acc += avg[n];
            }
            acc /= fft_len;

            //for (int i = 0; i != acc/10000; i++) {console_printf_e(" ");}console_printf("#");

            if ((prevpeak > threshold) && (acc < (prevpeak*9/10))) {
                ofdm_cplx_decode_symbol(i_eq, q_eq, i_symbol, q_symbol, fft_len);

                for (int n = -OFDM_CARRIER_PAIRS; n <= OFDM_CARRIER_PAIRS; n++) {
                    int idx = ofdm_carrier_to_idx(n, fft_len);
                    int ii = i_symbol[idx];
                    int qq = q_symbol[idx];

                    // Extracting the per-carrier equalization coefficients
                    // https://www.youtube.com/watch?v=CJsmsBUhW3c
                    cplx_inv(&ii, &qq, symbolampl * 2); // This works with a broad amplitude range

                    i_eq[idx] = ii;
                    q_eq[idx] = qq;
                }

                statemachine += 1;
                wp = 0;
                i_baseband[wp] = i_b;
                q_baseband[wp] = q_b;
                wp += 1;

            } else {

                prevpeak = acc;
                wp = (wp + 1) % fft_len;

                memmove(i_eq, &(i_eq[1]), ((fft_len-1) * sizeof(int)));
                memmove(q_eq, &(q_eq[1]), ((fft_len-1) * sizeof(int)));
                i_eq[fft_len - 1] = i_b;
                q_eq[fft_len - 1] = q_b;
            }
            break;

        default:

            i_baseband[wp] = i_a;
            q_baseband[wp] = q_a;

            wp += 1;
            if (wp == fft_len) {
                ofdm_cplx_decode_symbol(i_baseband, q_baseband, i_symbol, q_symbol, fft_len);
                for (int n = -OFDM_CARRIER_PAIRS; n <= OFDM_CARRIER_PAIRS; n++) {
                    int idx = ofdm_carrier_to_idx(n, fft_len);

                    cplx_mul(&(i_symbol[idx]),
                             &(q_symbol[idx]),
                                i_eq[idx],
                                q_eq[idx],
                                symbolampl);
                }
                uint8_t c = ofdm_symbol_to_u8(i_symbol, q_symbol, NULL, NULL, fft_len);
                //console_printf("0x%02x, [%c]", c, c);

                p->byte[pp++] = c;
                if (pp > OFDM_PKT_TOTAL_LEN(p->h.len)) {
                    statemachine = 0; // lost it due to invalid length
                    console_printf("invalid len");
                } else if (pp == OFDM_PKT_TOTAL_LEN(p->h.len)) {
                    if (ofdm_depacketize(p, NULL) < 0) {
                        statemachine = 0; // lost it due to CRC error
                        console_printf("crc error");
                    } else {
                        running = 0; // OK, return
                    }
                }
                wp = 0;
            }
            break;
        }
    }

	t_free(tap);
	t_free(buf_i);
	t_free(buf_q);

	t_free(avg);
	t_free(i_eq);
	t_free(q_eq);
	t_free(i_symbol);
	t_free(q_symbol);
	t_free(i_baseband);
	t_free(q_baseband);
    dds_destroy(mixer);

	return 1;
}





int setup_persona_commands (void) {

	keyword_add("rx", "- test", cmd_ofdm_rx);
	keyword_add("tx", "- test", cmd_ofdm_tx);
	keyword_add("dacsnk", "->DAC", cmd_dacsink);
	keyword_add("adcsrc", "ADC [fs]->", cmd_adcsrc);

	return 0;
}

