#include "dsp_maths.h"
#include "hal_plat.h" //malloc
#include <string.h> // memset


int isqrt (int y) {
    int left = 0;
    int right = y + 1;

    while (left < (right - 1)) {
        long long int mid = (left + right) / 2;

        if ((mid * mid) <= y) {
            left = mid;
        } else {
            right = mid;
        }
    }
    return left;
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


int magnitude_const (void) {
 //   return cos_func(0,1);
	return 2048;
}


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

		 *i_out += ((in[sample] * cos_func(s_idx, length)) / magnitude_const());
		 *q_out -= ((in[sample] * sin_func(s_idx, length)) / magnitude_const());
	 }
	 *i_out /= length;
	 *q_out /= length;
}


/* Complex Discrete Fourier Transform */
void dft_bucket_iq (int *in_i, int *in_q, int *i_out, int *q_out, int bucket, int length) {
    int sample;
    int mag = magnitude_const();

	*i_out = 0;
    *q_out = 0;

	for (sample = 0; sample != length; sample++) {
		 int s_idx = (bucket * sample) % length;

		 *i_out += (in_i[sample] * cos_func(s_idx, length));
		 *i_out += (in_q[sample] * sin_func(s_idx, length));
		 *q_out -= (in_i[sample] * sin_func(s_idx, length));
		 *q_out += (in_q[sample] * cos_func(s_idx, length));
	 }
	 *i_out /= (length * mag);
	 *q_out /= (length * mag);
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
    if ((*i > 45000) || (*q > 45000) || (*i < -45000) || (*q < -45000)) {
        console_printf("mul 1 overflow");
    }
    if ((i_b > 45000) || (q_b > 45000) || (i_b < -45000) || (q_b < -45000)) {
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

    if ((*i > 45000) || (*q > 45000) || (*i < -45000) || (*q < -45000)) {
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


dds_t* dds_create (int fs, int fc) {
	dds_t *instance = (dds_t*) t_malloc(sizeof(dds_t));
	if (!instance) {
		return NULL;
	}
	instance->phaseshift = (uint32_t)((268435456.0f / ((float)(fs / 16))) * (float)fc);
	dds_reset(instance);
	return instance;
}

void dds_destroy (dds_t* instance) {
	if (instance) {
		t_free(instance);
	}
}

void dds_reset (dds_t* instance) {
	instance->phaseaccumulator = 0;
}

void dds_next_sample (dds_t* instance, int *i, int *q) {
	*i = sinewave[(instance->phaseaccumulator) >> 24];
	*q = sinewave[(instance->phaseaccumulator + 0x40000000) >> 24];
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
    int centertap = (n * bf) -1;
    tap = (int*)t_malloc(fir_ntaps(n, bf) * sizeof(int));
    for (int i = 0; i != (n * bf); i++) {
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
