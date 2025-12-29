#include "analog.h"
#include "hal_plat.h" //malloc
#include <string.h> // memset


int isqrt (int y) {
    int left = 0;
    int right = y + 1;

    while (left != (right - 1)) {
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
		return (sin_func(n+1, samples) * ((samples * 2)+1) / (samples * 2)); // XXX this is not correct
	}
}


/* Discrete Fourier Transform */
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



void dft_bucket_iq2 (int *in_i, int *in_q, int *i_out, int *q_out, int bucket, int length, int startpos) {
    int sample;
    int mag = magnitude_const();

	*i_out = 0;
    *q_out = 0;

	for (sample = 0; sample != length; sample++) {
		 int sampleidx = (sample + startpos) % length;
		 int s_idx = (bucket * sample) % length;

		 *i_out += (in_i[sampleidx] * cos_func(s_idx, length));
		 *i_out += (in_q[sampleidx] * sin_func(s_idx, length));
		 *q_out -= (in_i[sampleidx] * sin_func(s_idx, length));
		 *q_out += (in_q[sampleidx] * cos_func(s_idx, length));
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



/* Complex Discrete Inverse Fourier Transform */
void ift_bucket_iq (int in_i, int in_q, int *i_out, int *q_out, int bucket, int length) {
	int sample;

    memset(i_out, 0x00, length * sizeof(int));
    memset(q_out, 0x00, length * sizeof(int));

	for (sample = 0; sample != length; sample++) {
		 int s_idx = (bucket * sample) % length;

		 i_out[sample] += (in_i * cos_func(s_idx, length));
		 i_out[sample] -= (in_q * sin_func(s_idx, length));
		 q_out[sample] += (in_i * sin_func(s_idx, length));
		 q_out[sample] += (in_q * cos_func(s_idx, length));
	 }
	 *i_out /= length;
	 *q_out /= length;
}



int magnitude (int i, int q) {
	return isqrt((i * i) + (q * q));
}


/* ================================ */



int ofdm_carrier_to_idx (int n, int samples) {
	return (samples + n) % samples;
}


int ofdm_cplx_encode_u8 (uint8_t c, int pilot_i, int pilot_q, int *i_out, int *q_out, int samples, int dynamic_range) {
	const int n_carriers = (OFDM_CARRIER_PAIRS * 2) + 1; // 4 + pilot
	int symbolampl = (samples * dynamic_range) / (n_carriers * (magnitude_const() * 2));

	int *i = (int*)t_malloc(samples * sizeof(int));
	int *q = (int*)t_malloc(samples * sizeof(int));

	memset(i, 0x00, samples * sizeof(int));
	memset(q, 0x00, samples * sizeof(int));

	for (int idx = -OFDM_CARRIER_PAIRS; idx <= OFDM_CARRIER_PAIRS; idx++) {
		int n = ofdm_carrier_to_idx(idx, samples);
		if (n) {
		    i[n] = ((c & 0x80) ? symbolampl : -symbolampl); c <<= 1;
		    q[n] = ((c & 0x80) ? symbolampl : -symbolampl); c <<= 1;
        }
	}
    i[0] = pilot_i * symbolampl;
    q[0] = pilot_q * symbolampl;

	for (int n = 0; n != samples; n++) {
		ift_sample_iq(i, q, &(i_out[n]), &(q_out[n]), n, samples);
	}
	t_free(q);
	t_free(i);

	return symbolampl;
}


uint8_t ofdm_cplx_decode_u8 (int *i_in, int *q_in, int *pilot_i, int *pilot_q, int samples) {
	uint8_t sample = 0x80;
	uint8_t b = 0x00;

    for (int idx = -OFDM_CARRIER_PAIRS; idx <= OFDM_CARRIER_PAIRS; idx++) {
    	int i;
    	int q;
    	int n = ofdm_carrier_to_idx(idx, samples);
		dft_bucket_iq(i_in, q_in, &i, &q, n, samples);
		if (n) {
			b |= (i > 0) ? sample : 0x00;  sample >>= 1;
			b |= (q > 0) ? sample : 0x00;  sample >>= 1;
		} else {
			if (pilot_i) *pilot_i = i;
			if (pilot_q) *pilot_q = q;
		}
	}
    return b;
}


uint8_t ofdm_cplx_decode_u8_2 (int *i_in, int *q_in, int *pilot_i, int *pilot_q, int samples, int start) {
	uint8_t sample = 0x80;
	uint8_t b = 0x00;

    for (int idx = -OFDM_CARRIER_PAIRS; idx <= OFDM_CARRIER_PAIRS; idx++) {
    	int i;
    	int q;
    	int n = ofdm_carrier_to_idx(idx, samples);
    	dft_bucket_iq2(i_in, q_in, &i, &q, n, samples, start);
		if (n) {
			b |= (i > 0) ? sample : 0x00;  sample >>= 1;
			b |= (q > 0) ? sample : 0x00;  sample >>= 1;
		} else {
			if (pilot_i) *pilot_i = i;
			if (pilot_q) *pilot_q = q;
		}
	}
    return b;
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


void cplx_upconvert (dds_t* dds, int *i, int *q, int* wave, int samples) {
	for (int n = 0; n != samples; n++) {
		int i_mix;
		int q_mix;
		dds_next_sample(dds, &i_mix, &q_mix);
		wave[n] = ((i[n] * i_mix) + (q[n] * q_mix)) / magnitude_const();
	}
}


void cplx_downconvert (dds_t* dds, int* wave, int *i, int *q, int samples) {
	for (int n = 0; n != samples; n++) {
		int i_mix;
		int q_mix;
		dds_next_sample(dds, &i_mix, &q_mix);
		i[n] = wave[n] * i_mix / magnitude_const();
		q[n] = wave[n] * q_mix / magnitude_const();
	}
}


void cplx_downconvert2 (dds_t* dds, int* wave, int *i, int *q, int samples, int startpos) {
	for (int n = 0; n != samples; n++) {
		int idx = (n + startpos) % samples;
		int i_mix;
		int q_mix;
		dds_next_sample(dds, &i_mix, &q_mix);
		i[n] = wave[idx] * i_mix / magnitude_const();
		q[n] = wave[idx] * q_mix / magnitude_const();
	}
}

