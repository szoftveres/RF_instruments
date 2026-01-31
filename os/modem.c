#include "modem.h"
#include "hal_plat.h"
#include "dsp_maths.h"
#include <string.h>


#define OFDM_BITS_PER_CARRIER (2)
#define OFDM_BYTES_PER_SYMBOL (1)
#define OFDM_CARRIER_PAIRS (OFDM_BITS_PER_CARRIER * OFDM_BYTES_PER_SYMBOL)



#define OFDM_FS (20000)
#define OFDM_FC (2000)

int ofdm_carrier_to_idx (int n, int samples) {
    return (samples + n) % samples;
}


int ofdm_cplx_u8_symbolampl (int samples, int dynamic_range) {
    const int n_carriers = (OFDM_CARRIER_PAIRS * 2) + 1; // 4 + pilot
    // 3/2 is an approximation for sqrt(2)
    return (samples * dynamic_range) / (n_carriers * (3/2) * (magnitude_const() * 2));
}


void ofdm_u8_to_symbol (uint8_t c, int pilot_i, int pilot_q, int *i_symbol, int *q_symbol, int samples, int symbolampl) {

    memset(i_symbol, 0x00, samples * sizeof(int));
    memset(q_symbol, 0x00, samples * sizeof(int));

    for (int idx = -OFDM_CARRIER_PAIRS; idx <= OFDM_CARRIER_PAIRS; idx++) {
        int n = ofdm_carrier_to_idx(idx, samples);
        if (n) {
            i_symbol[n] = ((c & 0x80) ? symbolampl : -symbolampl); c <<= 1;
            q_symbol[n] = ((c & 0x80) ? symbolampl : -symbolampl); c <<= 1;
        }
    }
    i_symbol[0] = pilot_i * symbolampl;
    q_symbol[0] = pilot_q * symbolampl;
}


uint8_t ofdm_symbol_to_u8 (int *i_symbol, int *q_symbol, int *pilot_i, int *pilot_q, int samples) {
    uint8_t sample = 0x80;
    uint8_t b = 0x00;

    for (int idx = -OFDM_CARRIER_PAIRS; idx <= OFDM_CARRIER_PAIRS; idx++) {
        int n = ofdm_carrier_to_idx(idx, samples);
        if (n) {
            b |= (i_symbol[n] > 0) ? sample : 0x00;  sample >>= 1;
            b |= (q_symbol[n] > 0) ? sample : 0x00;  sample >>= 1;
        } else {
            if (pilot_i) *pilot_i = i_symbol[n];
            if (pilot_q) *pilot_q = q_symbol[n];
        }
    }
    return b;

}


void ofdm_cplx_encode_symbol (int* i_symbol, int* q_symbol, int *i_out, int *q_out, int samples) {
    for (int n = 0; n != samples; n++) {
        ift_sample_iq(i_symbol, q_symbol, &(i_out[n]), &(q_out[n]), n, samples);
    }
}



void ofdm_cplx_decode_symbol (int *i_in, int *q_in, int* i_symbol, int* q_symbol, int samples) {
    for (int idx = -OFDM_CARRIER_PAIRS; idx <= OFDM_CARRIER_PAIRS; idx++) {
        int n = ofdm_carrier_to_idx(idx, samples);
        dft_bucket_iq(i_in, q_in, &(i_symbol[n]), &(q_symbol[n]), n, samples);
    }
}


uint8_t scrambler (uint8_t *core, uint8_t c) {
    int cycles = 8;
    while (cycles--) {
        *core = ((*core) << 1) + ((((*core) >> 3) ^ ((*core) >> 6)) & 0x01);
    }
    return (c ^ (*core));
}


#define OFDM_ANTIPREAMBLE (0x55)

#define OFDM_PKT_TOTAL_LEN(l) (((int)(l)) + ((int)sizeof(ofdm_pkt_header_t)))

int ofdm_packetize (ofdm_pkt_t* p, void *data, int len) {
    if (len > MODEM_PKT_PAYLOAD_MAX) {
        return 0;
    }
    p->h.len = (uint8_t)len;
    p->h.crc = 0;
    p->h.antipreamble = OFDM_ANTIPREAMBLE;
    if (len) {
        memcpy(p->payload, data, len);
    }
    p->h.crc = crc16(p->byte, OFDM_PKT_TOTAL_LEN(p->h.len));
    return len;
}


int ofdm_depacketize (ofdm_pkt_t* p, void **data) {
    uint16_t crc = 0;
    int rc = p->h.len;
    if (p->h.len > MODEM_PKT_PAYLOAD_MAX) {
        return -1;
    }
    if (data) {
        *data = p->payload;
    }
    crc = p->h.crc;
    p->h.crc = 0;
    if ((!crc) || (crc16(p->byte, OFDM_PKT_TOTAL_LEN(p->h.len)) != crc)) {
        rc = -1;
    }
    p->h.crc = crc;
    return rc;
}



#define OFDM_TX_OVERSAMPLE_RATE (8)
#define OFDM_RX_OVERSAMPLE_RATE (8)
#define OFDM_MODEM_SPS (100)

#define OFDM_TRAINING_FREQUENCY (4)
#define PFXLEN (fft_len/4)

void ofdm_txsymbol (dds_t *mixer, int *i_symbol, int *q_symbol, int *i_baseband, int *q_baseband, int dec, int len, int pfx) {
    int wave;
    int16_t sample_out;
    int i_a;
    int q_a;

    ofdm_cplx_encode_symbol(i_symbol, q_symbol, i_baseband, q_baseband, len);

    for (int f = 0; f != pfx; f += 1) {
        int idx = len - (pfx - f);
        for (int d = 0; d != dec; d += 1) {
            dds_next_sample(mixer, &i_a, &q_a);
            wave = ((i_baseband[idx] * i_a) + (q_baseband[idx] * q_a)) / magnitude_const();
            sample_out = (int16_t)wave;
            play_int16_sample(&sample_out);
        }
    }
    for (int f = 0; f != len; f += 1) {
        for (int d = 0; d != dec; d += 1) {
            dds_next_sample(mixer, &i_a, &q_a);
            wave = ((i_baseband[f] * i_a) + (q_baseband[f] * q_a)) / magnitude_const();
            sample_out = (int16_t)wave;
            play_int16_sample(&sample_out);
        }
    }
}


void ofdm_tx_noise (dds_t *mixer, int dec, int len, int symbolampl) {
	int i_a;
	int q_a;
	for (int f = 0; f != len; f += 1) {
		for (int d = 0; d != dec; d += 1) {
			dds_next_sample(mixer, &i_a, &q_a);
			int wave = (rand()%symbolampl) - (symbolampl/2);
			int16_t sample_out = (int16_t)wave;
			play_int16_sample(&sample_out);
		}
	}
}


void generate_training_symbol (int *i_symbol, int *q_symbol, int len, int symbolampl) {
    for (int n = -OFDM_CARRIER_PAIRS; n <= OFDM_CARRIER_PAIRS; n++) {
        int idx = ofdm_carrier_to_idx(n, len);
        i_symbol[ofdm_carrier_to_idx(idx, len)] = symbolampl;
        q_symbol[ofdm_carrier_to_idx(idx, len)] = 0;
    }
}


int ofdm_txpkt (ofdm_pkt_t *p) {
    int fs = OFDM_FS;
    int fft_len = ((OFDM_CARRIER_PAIRS * 2) + 1 ) * OFDM_TX_OVERSAMPLE_RATE; // carrier pairs * Nyquist * Oversample rate
    int target_fs = fft_len * OFDM_MODEM_SPS;
    int dec = fs / target_fs;
    int preambles = 4;
    int symbolampl = ofdm_cplx_u8_symbolampl(fft_len, 32768);

    int training_due = 0;
    uint8_t scrambler_core = 0xFF;

    start_audio_out(fs);

    dds_t *mixer = dds_create(fs, OFDM_FC);

    int *i_symbol = (int*)t_malloc(fft_len * sizeof(int));
    int *q_symbol = (int*)t_malloc(fft_len * sizeof(int));
    memset(i_symbol, 0x00, fft_len * sizeof(int));
    memset(q_symbol, 0x00, fft_len * sizeof(int));

    int *i_baseband = (int*)t_malloc(fft_len * sizeof(int));
    int *q_baseband = (int*)t_malloc(fft_len * sizeof(int));

    // Sending some noise
    for (int t = 0; t != 2; t++) {
        ofdm_tx_noise (mixer, dec, fft_len,  4096);
    }

    // Sending preamble
    for (int t = 0; t != preambles; t++) {
        generate_training_symbol(i_symbol, q_symbol, fft_len, -symbolampl);
        ofdm_txsymbol(mixer, i_symbol, q_symbol, i_baseband, q_baseband, dec, fft_len, 0);
    }

    int pp = 0;
    int pilot = 1;
    while (pp != OFDM_PKT_TOTAL_LEN(p->h.len)) {
        if (!training_due) {
            training_due = OFDM_TRAINING_FREQUENCY;
            generate_training_symbol(i_symbol, q_symbol, fft_len, symbolampl);
        } else {
            training_due -= 1;
            ofdm_u8_to_symbol(scrambler(&scrambler_core, p->byte[pp++]), pilot, 0, i_symbol, q_symbol, fft_len, symbolampl);
            pilot *= -1; // Alternating the pilot, to discourage the Rx autocorrelator of syncing mid-packet
        }
        ofdm_txsymbol(mixer, i_symbol, q_symbol, i_baseband, q_baseband, dec, fft_len, PFXLEN);
    }

    // Sending some noise
    for (int t = 0; t != 2; t++) {
        ofdm_tx_noise (mixer, dec, fft_len,  4096);
    }


    t_free(i_symbol);
    t_free(q_symbol);
    t_free(i_baseband);
    t_free(q_baseband);
    dds_destroy(mixer);

    stop_audio_out();

    return 1;
}


int save_training_eq (int *i_symbol, int *q_symbol, int *i_eq, int *q_eq, int symbolampl, int len) {
    for (int n = -OFDM_CARRIER_PAIRS; n <= OFDM_CARRIER_PAIRS; n++) {
        int idx = ofdm_carrier_to_idx(n, len);
        int ii = i_symbol[idx];
        int qq = q_symbol[idx];

        // Extracting the per-carrier equalization coefficients
        // https://www.youtube.com/watch?v=CJsmsBUhW3c
        if (cplx_inv(&ii, &qq, symbolampl * 256)) { // This works with a broad amplitude range
            return -1;
        }

        i_eq[idx] = ii;
        q_eq[idx] = qq;
    }
    return 0;
}



int ofdm_rxpkt (ofdm_pkt_t *p) {
    int fs = OFDM_FS;
    int rc;
    int fft_len = ((OFDM_CARRIER_PAIRS * 2) + 1 ) * OFDM_RX_OVERSAMPLE_RATE; // carrier pairs * Nyquist * Oversample rate
    int target_fs = fft_len * OFDM_MODEM_SPS;
    int dec = fs / target_fs;
    int symbolampl = ofdm_cplx_u8_symbolampl(fft_len, 32768);

    start_audio_in(fs);

    dds_t *mixer = dds_create(fs, OFDM_FC);

    int min = 1024 * 1024;
    int max = -min;
    int *i_symbol = (int*)t_malloc(fft_len * sizeof(int));
    int *q_symbol = (int*)t_malloc(fft_len * sizeof(int));
    int *i_eq = (int*)t_malloc(fft_len * sizeof(int));
    int *q_eq = (int*)t_malloc(fft_len * sizeof(int));
    int *i_baseband = (int*)t_malloc(fft_len * sizeof(int));
    int *q_baseband = (int*)t_malloc(fft_len * sizeof(int));

    int training_due;
    uint8_t scrambler_core = 0xFF;

    int16_t sample_in;
    int wave;
    int running = 1;
    int i_a;
    int q_a;
    int pp;
    memset(i_symbol, 0x00, fft_len * sizeof(int));
    memset(q_symbol, 0x00, fft_len * sizeof(int));
    memset(i_eq, 0x00, fft_len * sizeof(int));
    memset(q_eq, 0x00, fft_len * sizeof(int));

    int *correlator_i = (int*)t_malloc(fft_len * sizeof(int));
    memset(correlator_i, 0x00, fft_len * sizeof(int));
    int acc_i = 0;
    int *correlator_q = (int*)t_malloc(fft_len * sizeof(int));
    memset(correlator_q, 0x00, fft_len * sizeof(int));
    int acc_q = 0;
    int *corravg = (int*)t_malloc(fft_len * sizeof(int));
    memset(corravg, 0x00, fft_len * sizeof(int));
    int corravg_acc = 0;

    int *sigpwr = (int*)t_malloc(fft_len * sizeof(int));
    memset(sigpwr, 0x00, fft_len * sizeof(int));
    int sigpwr_acc = 0;

    // Setting up the decimating filter
    int taps = fir_ntaps(dec, 2);
    int *tap = fir_create_taps(dec, 2);
    int norm = fir_normf(tap, taps);
    int *buf_i = (int*)t_malloc(taps * sizeof(int));
    int *buf_q = (int*)t_malloc(taps * sizeof(int));
    int bp = 0;
    int wp = 0;
    int statemachine = 0;
    int pfx;
    int threshold = 256 * 1;

    while (running) {

        dds_next_sample(mixer, &i_a, &q_a);
        record_int16_sample(&sample_in); wave = ((int)sample_in);

        if (wave < min) {min = wave;}
        if (wave > max) {max = wave;}

        buf_i[bp] = wave * i_a / magnitude_const();
        buf_q[bp] = wave * q_a / magnitude_const();
        bp++;
        if (bp < taps) {
            continue;
        }
        bp -= dec;  // due to memmove in fir_work
        i_a = fir_work(buf_i, tap, taps, dec) / norm / 4;
        q_a = fir_work(buf_q, tap, taps, dec) / norm / 4;
        switch (statemachine) {
        case 0:
            int lcl_acc_i;
            int lcl_acc_q;

            // delay line
            int delayline_i = i_eq[wp];
            int delayline_q = q_eq[wp];
            i_eq[wp] = i_a;
            q_eq[wp] = q_a;

            cplx_mul(&i_a, &q_a, delayline_i, -delayline_q, symbolampl);

            // Preamble autocorrelator moving average
            acc_i -= correlator_i[wp];
            correlator_i[wp] = i_a;
            acc_i += correlator_i[wp];

            acc_q -= correlator_q[wp];
            correlator_q[wp] = q_a;
            acc_q += correlator_q[wp];

            // Signal power meter moving average
            sigpwr_acc -= sigpwr[wp];
            sigpwr[wp] = (delayline_i * delayline_i / symbolampl) + (delayline_q * delayline_q / symbolampl);
            sigpwr_acc += sigpwr[wp];

            lcl_acc_i = acc_i / fft_len;
            lcl_acc_q = acc_q / fft_len;
            int sigpwr = sigpwr_acc / fft_len;

            if (!sigpwr) {
                break;
            }

            int magn = ((lcl_acc_i * lcl_acc_i / sigpwr) + (lcl_acc_q * lcl_acc_q / sigpwr));
            // magn /= sigpwr;  <-  This will be correct

            // Correlator magnitude delay line
            corravg_acc -= corravg[wp];
            corravg[wp] = magn;
            corravg_acc += corravg[wp];
            int lcl_corravg = corravg_acc / fft_len;

            //console_printf("magn:%i, magn_avg:%i, sigpwr:%i", magn, lcl_corravg, sigpwr);
            // A drop in correlation between the current value (compared to the value at the previous symbol)
            // inndicates the end of the preamble
            if ((lcl_corravg > threshold) && (magn < (lcl_corravg / 2))) {
                console_printf("sigpwr:%i (%i->%i)", sigpwr, lcl_corravg, magn);

                statemachine += 1;
                wp = 0;
                pp = 0;
                training_due = 0;
                // The trigger occurs somewhere in the early section of the cyclic prefix,
                // shooting for the middle
                pfx = PFXLEN * 1 / 4;
                p->h.len = MODEM_PKT_PAYLOAD_MAX;
                p->h.antipreamble = OFDM_ANTIPREAMBLE;
            } else {
                wp = (wp + 1) % fft_len;
            }
            break;
        default:

            if (pfx) {
                pfx -= 1;
            } else {
                i_baseband[wp] = i_a;
                q_baseband[wp] = q_a;
                wp += 1;
            }
            if (wp == fft_len) {
                pfx = PFXLEN;
                wp = 0;
                ofdm_cplx_decode_symbol(i_baseband, q_baseband, i_symbol, q_symbol, fft_len);
                if (!training_due) {
                    save_training_eq(i_symbol, q_symbol, i_eq, q_eq, symbolampl, fft_len);
                    training_due = OFDM_TRAINING_FREQUENCY;
                } else {
                    training_due -= 1;

                    for (int n = -OFDM_CARRIER_PAIRS; n <= OFDM_CARRIER_PAIRS; n++) {
                        int idx = ofdm_carrier_to_idx(n, fft_len);

                        cplx_mul(&(i_symbol[idx]),
                                 &(q_symbol[idx]),
                                    i_eq[idx],
                                    q_eq[idx],
                                    symbolampl);

                    }
                    uint8_t c = ofdm_symbol_to_u8(i_symbol, q_symbol, NULL, NULL, fft_len);

                    p->byte[pp++] = scrambler(&scrambler_core, c);

                    if (p->h.len > MODEM_PKT_PAYLOAD_MAX) {
                        statemachine = 0; // lost it due to garbage length
                        running = 0;
                        rc = -1;
                    }
                    if (p->h.antipreamble != OFDM_ANTIPREAMBLE) {
                        statemachine = 0; // lost it due garbage data (sync issue probably)
                        running = 0;
                        rc = -1;
                    }
                    if (pp >= OFDM_PKT_TOTAL_LEN(p->h.len)) {
                        if (ofdm_depacketize(p, NULL) < 0) {
                            statemachine = 0; // lost it due to CRC error
                            running = 0;
                            rc = -1;
                        } else {
                            running = 0; // OK, return
                            rc = p->h.len;
                        }
                    }
                }
            }
            break;
        }
    }

    stop_audio_in();

    t_free(tap);
    t_free(buf_i);
    t_free(buf_q);

    t_free(sigpwr);
    t_free(corravg);
    t_free(correlator_i);
    t_free(correlator_q);
    t_free(i_eq);
    t_free(q_eq);
    t_free(i_symbol);
    t_free(q_symbol);
    t_free(i_baseband);
    t_free(q_baseband);
    dds_destroy(mixer);

    return rc;
}




/* ============== D-BPSK ===================== */


#define BPSK_FS (8000)
#define BPSK_FC (2000)



// (Barker 4) x -(Barker 4)
#define BPSK_PKT_PREAMBLE1 (0xB4)
// (Barker 5) x -(Barker 3)
#define BPSK_PKT_PREAMBLE2 (0xE9)


#define BPSK_PKT_TOTAL_LEN(l) (((int)(l)) + ((int)sizeof(bpsk_pkt_header_t)))

int bpsk_packetize (bpsk_pkt_t* p, void *data, int len) {
    if (len > MODEM_PKT_PAYLOAD_MAX) {
        return 0;
    }
    p->h.len = (uint8_t)len;
    p->h.crc = 0;
    p->h.preamble1 = BPSK_PKT_PREAMBLE1;
    p->h.preamble2 = BPSK_PKT_PREAMBLE2;
    if (len) {
        memcpy(p->payload, data, len);
    }
    p->h.crc = crc16(p->byte, BPSK_PKT_TOTAL_LEN(p->h.len));
    return len;
}

int bpsk_depacketize (bpsk_pkt_t* p, void **data) {
    uint16_t crc = 0;
    if (p->h.len > MODEM_PKT_PAYLOAD_MAX) {
        return -1;
    }
    if (data) {
        *data = p->payload;
    }
    crc = p->h.crc;
    p->h.crc = 0;
    if (crc16(p->byte, BPSK_PKT_TOTAL_LEN(p->h.len)) != crc) {
        return -1;
    }
    p->h.crc = crc;
    return p->h.len;
}


#define BPSK_BAUDRATE (400)
#define BPSK_RX_OVERSAMPLE_RATE (5)

typedef struct bpsk_context_s {
    int fft_len;
    dds_t* mixer;
    int* i;
    int* q;
    int wp;
    int sync;

    uint8_t scrambler_core;

    int cm;
    int cmi;

    int pp;

    uint8_t byte[BPSK_RX_OVERSAMPLE_RATE];
    int bitcounter;

    struct {  //         Decimating filter
        int *buf_i;
        int *buf_q;
        int bp;          // This tracks the incoming samples

        int *tap;
        int taps;
        int norm; // sum of taps, for normalization

        int target_fs;
        int dec; // decimation factor
    } ds;

    int* bitval;

} bpsk_context_t;



int bpsk_rxpkt (bpsk_pkt_t *p) {
    int16_t sample;
    bpsk_context_t c;
    int fs = BPSK_FS;

    start_audio_in(fs);

    memset(&c, 0x00, sizeof(bpsk_context_t));
    c.scrambler_core = 0xFF;
    c.mixer = NULL; // yet unknown, fs depends on the sample rate

    c.fft_len = BPSK_RX_OVERSAMPLE_RATE; // carrier pairs * Nyquist * Oversample rate

    c.i = (int*)t_malloc(c.fft_len * sizeof(int));
    c.q = (int*)t_malloc(c.fft_len * sizeof(int));
    c.bitval = (int*)t_malloc(c.fft_len * sizeof(int));

    c.ds.target_fs = c.fft_len * BPSK_BAUDRATE;

    c.ds.dec = 0;
    p->h.len = MODEM_PKT_PAYLOAD_MAX;

    c.mixer = dds_create(fs, BPSK_FC);

    c.ds.dec = fs / c.ds.target_fs; //  dec =   fs / target fs

    c.ds.taps = fir_ntaps(c.ds.dec, 2);
    c.ds.tap = fir_create_taps(c.ds.dec, 2);
    c.ds.norm = fir_normf(c.ds.tap, c.ds.taps);
    c.ds.buf_i = (int*)t_malloc(c.ds.taps * sizeof(int));
    c.ds.buf_q = (int*)t_malloc(c.ds.taps * sizeof(int));

    c.wp = 0;
    c.ds.bp = 0;
    c.pp = -1;


    int rss;
    int rc = -1;

    while (1) {
        // Downconverting to baseband
        int i_a;
        int q_a;
        dds_next_sample(c.mixer, &i_a, &q_a);
        record_int16_sample(&sample);
        c.ds.buf_i[c.ds.bp] = ((int)sample) * i_a / magnitude_const();
        c.ds.buf_q[c.ds.bp] = ((int)sample) * q_a / magnitude_const();

        if (c.pp == BPSK_PKT_TOTAL_LEN(p->h.len)) { // We're done
            rc = bpsk_depacketize(p, NULL);
            break;
        }

        c.ds.bp++;
        if (c.ds.bp < c.ds.taps) {
            continue;
        }
        c.ds.bp -= c.ds.dec;  // due to memmove in fir_work

        // Downsampling (lowpass and decimate)
        c.wp = (c.wp + 1) % c.fft_len;              // move WP

        i_a = c.i[c.wp];              // Delayed conjugate
        q_a = -(c.q[c.wp]);           // Delayed conjugate
        c.i[c.wp] = fir_work(c.ds.buf_i, c.ds.tap, c.ds.taps, c.ds.dec) / c.ds.norm;
        c.q[c.wp] = fir_work(c.ds.buf_q, c.ds.tap, c.ds.taps, c.ds.dec) / c.ds.norm;

        // Polar discriminator
        cplx_mul(&i_a, &q_a, c.i[c.wp], c.q[c.wp], magnitude_const());
        c.bitval[c.wp] = i_a;
        //c.bitval[c.wp] = ((i_a * c.i[c.wp]) - (q_a * c.q[c.wp])) / magnitude_const();
        // We only care about the real part after the polar discriminator

        int bit = 0;
        // matched filter
        for (int i = 0; i != c.fft_len; i++) {
            bit += c.bitval[i];
        }
        bit /= c.fft_len;
        rss = ((bit < 0) ? -bit : bit);
        c.byte[c.wp] <<= 1;
        c.byte[c.wp] |= (bit > 0 ? 0x00 : 0x01);

        switch (c.pp) {
            case -1: { // trying to sync to the preamble
                if (c.byte[c.wp] == BPSK_PKT_PREAMBLE1) {
                    //console_printf("sync start, rss:%i", rss);
                    c.sync = 0;
                    c.cm = rss;
                    c.cmi = c.sync;
                    c.pp += 1;  // Advance to the next state
                    p->byte[c.pp] = c.byte[c.wp];
                }
                continue;
            };
            case 0: { // Found a valid preamble, let's find the max
                c.sync += 1;
                if ((c.byte[c.wp] == BPSK_PKT_PREAMBLE1) && (rss > c.cm)) { // Found a better peak
                    //console_printf("sync        rss:%i", rss);
                    c.cm = rss;
                    c.cmi = c.sync;
                }
                if (c.sync > (c.fft_len - 2)) {
                    //console_printf("sync end    sync:%i", c.sync);
                    c.sync = (c.fft_len - c.sync) + c.cmi; // Samples to skip for best sync to the next symbol
                    c.bitcounter = 0;
                    //console_printf("sync end best sync:%i", c.sync);
                    c.pp += 1; // Advance to the next state
                }
                continue;
            };
        }

        c.sync -= 1;
        if (c.sync) { // Just sinking samples
            continue;
        }

        c.sync = c.fft_len;
        c.bitcounter++;
        if (c.bitcounter < 8) {
            continue;
        }
        c.bitcounter = 0;

        p->byte[c.pp] = c.byte[c.wp];
        c.cm += rss;
        c.sync = c.fft_len;
        if (((c.pp == 1) && (p->byte[c.pp] != BPSK_PKT_PREAMBLE2)) ||
            (p->h.len > MODEM_PKT_PAYLOAD_MAX)) {
            //console_printf("lost it: :0x%02X, rss:%i", p->byte[c.pp], rss);
            c.pp = -1; // We lost it
            c.wp = 0;
            continue;
        }
        c.pp += 1; // Advance to the next stage

        continue;
    }

    stop_audio_in();

    dds_destroy(c.mixer);
    t_free(c.i);
    t_free(c.q);
    t_free(c.bitval);
    t_free(c.ds.buf_i);
    t_free(c.ds.buf_q);

    return rc;
}


int bpsk_txpkt (bpsk_pkt_t *p) {
    int fs = BPSK_FS;
    bpsk_context_t c;
    memset(&c, 0x00, sizeof(bpsk_context_t));
    c.scrambler_core = 0xFF;

    start_audio_out(fs);

    c.mixer = dds_create(fs, BPSK_FC);
    c.fft_len = 1; // carrier pairs * Nyquist * Oversample rate
    c.i = (int*)t_malloc(c.fft_len * sizeof(int));
    memset(c.i, 0x00, c.fft_len * sizeof(int));
    c.q = (int*)t_malloc(c.fft_len * sizeof(int));
    memset(c.q, 0x00, c.fft_len * sizeof(int));

    c.ds.target_fs = c.fft_len * BPSK_BAUDRATE;
    c.ds.dec = fs / c.ds.target_fs;


    c.pp = -2;   // XXX should be -2
    c.bitcounter = 7;
    c.wp = 0;


    while (c.pp <= (BPSK_PKT_TOTAL_LEN(p->h.len) + 1)) {

        if (c.wp) { // Upconvert and play
            int16_t sample;
            c.wp -= 1;

            int i_mix;
            int q_mix;
            dds_next_sample(c.mixer, &i_mix, &q_mix);
            sample = (int16_t)(((c.i[c.ds.bp] * i_mix) + (c.q[c.ds.bp] * q_mix)) / magnitude_const());

            play_int16_sample(&sample);
            continue;
        }
        c.wp = c.ds.dec;


        c.ds.bp += 1;
        if (c.ds.bp < c.fft_len) { // Go to the next baseband (oversampled) sample
            continue;
        }
        c.ds.bp = 0;


        uint8_t nextbit = c.byte[c.ds.bp] & 0x80;
        c.byte[c.ds.bp] <<= 1;
        if (nextbit) { // differential encoding
            c.i[c.ds.bp] *= -1;
        }

        c.bitcounter++;
        if (c.bitcounter < 8) {
            continue;
        }
        c.bitcounter = 0;

        // Next byte
        c.pp += 1;
        if (c.pp < 0) {
            c.byte[c.ds.bp] = 0;
            c.i[c.ds.bp] = 16384;
            c.q[c.ds.bp] = 0;
        } else if (c.pp < BPSK_PKT_TOTAL_LEN(p->h.len)) {
            c.byte[c.ds.bp] = p->byte[c.pp];
        } else {
            c.byte[c.ds.bp] = 0;
        }
    }

    dds_destroy(c.mixer);
    t_free(c.i);
    t_free(c.q);

    stop_audio_out();

    return 0;
}



