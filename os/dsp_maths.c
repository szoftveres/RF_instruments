#include "dsp_maths.h"
#include "hal_plat.h" // t_malloc
#include <string.h> // memset
#include <limits.h> //int max and int min


// Searching for the square root by successive approximation

int isqrt (int y) {
    int sar = 0x00008000;
    int res = 0;

    while (sar) {
        res |= sar;
        if ((res * res) > y) {
            res &= (~sar);
        }
        sar >>= 1;
    }
    return res;
}


int ampldb (int lo, int hi) {
    int db = 0;

    if ((!lo) || (lo > hi)) {
    	return INT_MAX;
    }

    while ((lo * 10) <= hi) {
        lo *= 10;
        db += 20;
    }
    hi *= 10;
    if (lo * 89 < hi) { db += 1;} // 19 dB
    if (lo * 79 < hi) { db += 1;} // 18 dB
    if (lo * 71 < hi) { db += 1;} // 17 dB
    if (lo * 63 < hi) { db += 1;} // 16 dB
    if (lo * 56 < hi) { db += 1;} // 15 dB
    if (lo * 50 < hi) { db += 1;} // 14 dB
    if (lo * 45 < hi) { db += 1;} // 13 dB
    if (lo * 40 < hi) { db += 1;} // 12 dB
    if (lo * 35 < hi) { db += 1;} // 11 dB
    if (lo * 32 < hi) { db += 1;} // 10 dB
    if (lo * 28 < hi) { db += 1;} //  9 dB
    if (lo * 25 < hi) { db += 1;} //  8 dB
    if (lo * 22 < hi) { db += 1;} //  7 dB
    if (lo * 20 < hi) { db += 1;} //  6 dB
    if (lo * 18 < hi) { db += 1;} //  5 dB
    if (lo * 16 < hi) { db += 1;} //  4 dB
    if (lo * 14 < hi) { db += 1;} //  3 dB
    if (lo * 13 < hi) { db += 1;} //  2 dB
    if (lo * 11 < hi) { db += 1;} //  1 dB

    return db;
}


const int sinewave[] = {
          0,         50,        100,        151,
        201,        251,        300,        350,
        399,        449,        497,        546,
        594,        642,        690,        737,
        783,        830,        875,        920,
        965,       1009,       1052,       1095,
       1137,       1179,       1219,       1259,
       1299,       1337,       1375,       1411,
       1447,       1483,       1517,       1550,
       1582,       1614,       1644,       1674,
       1702,       1729,       1756,       1781,
       1805,       1828,       1850,       1871,
       1891,       1910,       1927,       1944,
       1959,       1973,       1986,       1997,
       2008,       2017,       2025,       2032,
       2037,       2041,       2045,       2046,
       2047,       2046,       2045,       2041,
       2037,       2032,       2025,       2017,
       2008,       1997,       1986,       1973,
       1959,       1944,       1927,       1910,
       1891,       1871,       1850,       1828,
       1805,       1781,       1756,       1729,
       1702,       1674,       1644,       1614,
       1582,       1550,       1517,       1483,
       1447,       1411,       1375,       1337,
       1299,       1259,       1219,       1179,
       1137,       1095,       1052,       1009,
        965,        920,        875,        830,
        783,        737,        690,        642,
        594,        546,        497,        449,
        399,        350,        300,        251,
        201,        151,        100,         50,
          0,        -50,       -100,       -151,
       -201,       -251,       -300,       -350,
       -399,       -449,       -497,       -546,
       -594,       -642,       -690,       -737,
       -783,       -830,       -875,       -920,
       -965,      -1009,      -1052,      -1095,
      -1137,      -1179,      -1219,      -1259,
      -1299,      -1337,      -1375,      -1411,
      -1447,      -1483,      -1517,      -1550,
      -1582,      -1614,      -1644,      -1674,
      -1702,      -1729,      -1756,      -1781,
      -1805,      -1828,      -1850,      -1871,
      -1891,      -1910,      -1927,      -1944,
      -1959,      -1973,      -1986,      -1997,
      -2008,      -2017,      -2025,      -2032,
      -2037,      -2041,      -2045,      -2046,
      -2047,      -2046,      -2045,      -2041,
      -2037,      -2032,      -2025,      -2017,
      -2008,      -1997,      -1986,      -1973,
      -1959,      -1944,      -1927,      -1910,
      -1891,      -1871,      -1850,      -1828,
      -1805,      -1781,      -1756,      -1729,
      -1702,      -1674,      -1644,      -1614,
      -1582,      -1550,      -1517,      -1483,
      -1447,      -1411,      -1375,      -1337,
      -1299,      -1259,      -1219,      -1179,
      -1137,      -1095,      -1052,      -1009,
       -965,       -920,       -875,       -830,
       -783,       -737,       -690,       -642,
       -594,       -546,       -497,       -449,
       -399,       -350,       -300,       -251,
       -201,       -151,       -100,        -50
};


#define LOOKUP_LEN    (sizeof(sinewave)/sizeof(sinewave[0]))


int sin_func (int n, int samples) {
    return sinewave[((n * LOOKUP_LEN) / samples) % LOOKUP_LEN];
}


int cos_func (int n, int samples) {
    return sinewave[(((n * LOOKUP_LEN) / samples) + (LOOKUP_LEN / 4)) % LOOKUP_LEN];
}


int raised_cos_window (int n, int samples) {
	return -cos_func(n, samples-1) + 2047;
}


int sine_window (int n, int samples) {
	return sin_func(n, (samples-1) * 2);
}


int sinc_func (int n, int samples) {
	if (n) {
		return (sin_func(n, samples) / n);
	} else {
		return (sin_func(n+1, samples) * ((samples * 2)+1) / (samples * 2));  // XXX this is not correct
	}
}


/* Discrete Fourier Transform of real waveform */
void dft_bucket (int *in, int *i_out, int *q_out, int bucket, int length) {
    int sample;

	*i_out = 0;
    *q_out = 0;

	for (sample = 0; sample != length; sample++) {
		 int s_idx = (bucket * sample) % length;

		 *i_out += ((in[sample] * cos_func(s_idx, length)) / MAGNITUDE_CONST);
		 *q_out -= ((in[sample] * sin_func(s_idx, length)) / MAGNITUDE_CONST);
	 }
	 *i_out /= length;
	 *q_out /= length;
}


/* Complex Discrete Fourier Transform */
void dft_bucket_iq (int *in_i, int *in_q, int *i_out, int *q_out, int bucket, int length) {
    int sample;

	*i_out = 0;
    *q_out = 0;

	for (sample = 0; sample != length; sample++) {
		 int s_idx = (bucket * sample) % length;

		 *i_out += (in_i[sample] * cos_func(s_idx, length));
		 *i_out += (in_q[sample] * sin_func(s_idx, length));
		 *q_out -= (in_i[sample] * sin_func(s_idx, length));
		 *q_out += (in_q[sample] * cos_func(s_idx, length));
	 }
	 *i_out /= (length * MAGNITUDE_CONST);
	 *q_out /= (length * MAGNITUDE_CONST);
}


/* Complex Discrete Inverse Fourier Transform */
void ift_sample_iq (int *in_i, int *in_q, int *i_out, int *q_out, int sample, int length) {
    int bucket;

	*i_out = 0;
    *q_out = 0;

	for (bucket = 0; bucket != length; bucket++) {
		 int s_idx = (sample * bucket) % length;

		 *i_out += (in_i[bucket] * cos_func(s_idx, length));
		 *i_out -= (in_q[bucket] * sin_func(s_idx, length));
		 *q_out += (in_i[bucket] * sin_func(s_idx, length));
		 *q_out += (in_q[bucket] * cos_func(s_idx, length));
	 }
	 *i_out /= length;
	 *q_out /= length;
}



int magnitude (int i, int q) {
	return isqrt((i * i) + (q * q));
}


/* ================================ */


void cplx_mul (int *i, int *q, int i_b, int q_b, int norm) {
    if ((*i > 49151) || (*q > 49151) || (*i < -49152) || (*q < -49152)) {
        console_printf("mul 1 overflow");
    }
    if ((i_b > 49151) || (q_b > 49151) || (i_b < -49152) || (q_b < -49152)) {
        console_printf("mul 2 overflow");
    }
    int dst_i = (*i * i_b) - (*q * q_b);
    int dst_q = (*i * q_b) + (*q * i_b);
    *i = dst_i / norm;
    *q = dst_q / norm;
}

void cplx_div (int *i, int *q, int i_b, int q_b, int norm) {
    int dst_i = ((*i * i_b) + (*q * q_b)) / ((i_b * i_b) + (q_b * q_b));
    int dst_q = ((*q * i_b) - (*i * q_b)) / ((i_b * i_b) + (q_b * q_b));
    *i = dst_i / norm;
    *q = dst_q / norm;
}

int cplx_inv (int *i, int *q, int norm) {

    if ((*i > 49151) || (*q > 49151) || (*i < -49152) || (*q < -49152)) {
        console_printf("inv overflow");
    }

    int d = ((*i * *i) + (*q * *q));
    if (!d) {
        return -1;
    }
    *i = (*i * norm) / d;
    *q = -(*q * norm) / d;
    return 0;
}

/* ================================ */


dds_t* dds_create (int fs, int fc, const int *wavetable) {
	dds_t *instance = (dds_t*) t_malloc(sizeof(dds_t));
	if (!instance) {
		return NULL;
	}
    instance->phaseshift = (uint32_t)((((uint64_t)fc) << 32) / (uint64_t)fs);
	instance->wavetable = wavetable;
	dds_reset(instance);
	return instance;
}

dds_t* dds_create_f (float fs, float fc, const int *wavetable) {
    dds_t *instance = (dds_t*) t_malloc(sizeof(dds_t));
    if (!instance) {
        return NULL;
    }
    instance->phaseshift = (uint32_t)((((double)fc) * 4294967296.0) / (double)fs);
    instance->wavetable = wavetable;
    dds_reset(instance);
    return instance;
}


void dds_destroy (dds_t* instance) {
	if (instance) {
		t_free(instance);
	}
}

void dds_set (dds_t* instance, uint8_t shift) {
    instance->phaseaccumulator = ((uint32_t)shift) << 24;
}

void dds_reset (dds_t* instance) {
	instance->phaseaccumulator = 0;
}

void dds_next_sample (dds_t* instance, int *i, int *q) {
	*q = instance->wavetable[(instance->phaseaccumulator) >> 24];
	*i = instance->wavetable[(instance->phaseaccumulator + 0x40000000) >> 24];
	instance->phaseaccumulator += instance->phaseshift;
}


int fir_normf (int tap[], int taps) {
    int n = 0;
    for (int i = 0; i != taps; i++) {
        n += tap[i];
    }
    return n;
}


int fir_ntaps (int n, int bf) {
    return (2 * n * bf) - 1;
}


int* fir_create_taps (int n, int bf) {
    int* tap;
    int taps = fir_ntaps(n, bf);
    int centertap = taps / 2;
    tap = (int*)t_malloc(taps * sizeof(int));
    for (int i = 0; i != centertap + 1; i++) {
        tap[centertap+i] = sinc_func(i, n*2);
        tap[centertap-i] = sinc_func(i, n*2);
    }
    return tap;
}


int fir_work (int buf[], int tap[], int taps, int dec) {
    int result = 0;
    for (int t = 0; t != taps; t += 1) {
        result += buf[t] * tap[t];
    }
    memmove(buf, &(buf[dec]), (taps-dec)*sizeof(int));
    return result;
}



uint8_t crc8 (uint8_t byte[], int len) {
    uint8_t crc = 0;

    for (int n = 0; n != len; n++) {
        crc ^= byte[n];

        for (int i = 0; i != 8; i++) {
            crc <<= 1;
            if (crc & 0x80) {
                crc ^= 0xE7;  // generator polynomial
            }
        }
    }
    return crc;
}


uint16_t crc16 (uint8_t byte[], int len) {
    uint16_t crc = 0;

    for (int n = 0; n != len; n++) {
        crc ^= ((uint16_t)byte[n]) << 8;

        for (int i = 0; i != 8; i++) {
            crc <<= 1;
            if (crc & 0x8000) {
                crc ^= 0xac9a;  // generator polynomial
            }
        }
    }
    return crc;
}

/* =========== */

moving_sum_t* moving_sum_create (int len) {
	moving_sum_t *instance = (moving_sum_t*) t_malloc(sizeof(moving_sum_t));
	if (!instance) {
		return NULL;
	}
    instance->queue = (int*)t_malloc(sizeof(int) * len);
	memset(instance->queue, 0x00, sizeof(int) * len);

	instance->len = len;
	instance->p = 0;
	instance->sum = 0;
	return instance;
}


void moving_sum_destroy (moving_sum_t* instance) {
	if (instance) {
		t_free(instance->queue);
		t_free(instance);
	}
}


int moving_average (moving_sum_t* instance, int val) {
	instance->sum -= instance->queue[instance->p];
	instance->queue[instance->p] = val;
	instance->sum += val;
	instance->p = (instance->p + 1) % instance->len;
	return instance->sum / instance->len;
}

int moving_sum (moving_sum_t* instance, int val) {
	instance->sum -= instance->queue[instance->p];
	instance->queue[instance->p] = val;
	instance->sum += val;
	instance->p = (instance->p + 1) % instance->len;
	return instance->sum;
}

int delayline (moving_sum_t* instance, int val) {
	int tailv = instance->queue[instance->p];
	instance->queue[instance->p] = val;
	instance->p = (instance->p + 1) % instance->len;
	return tailv;
}
