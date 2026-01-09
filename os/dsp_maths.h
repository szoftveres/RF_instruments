#ifndef __DSP_MATHS_H__
#define __DSP_MATHS_H__

#include <stdint.h> // uint32_t

int isqrt (int y);

int sin_func (int n, int samples);
int cos_func (int n, int samples);
int raised_cos_window (int n, int samples);
int sine_window (int n, int samples);
int sinc_func (int n, int samples);

void dft_bucket (int *in, int *i_out, int *q_out, int bucket, int length);
void dft_bucket_iq (int *in_i, int *in_q, int *i_out, int *q_out, int bucket, int length);
void ift_sample_iq (int *in_i, int *in_q, int *i_out, int *q_out, int sample, int length);

int magnitude (int i, int q);
int magnitude_const (void);


typedef struct dds_s {
	uint32_t phaseshift;
	uint32_t phaseaccumulator;
} dds_t;


void cplx_mul (int *i, int *q, int i_b, int q_b, int norm);
void cplx_div (int *i, int *q, int i_b, int q_b, int norm);
int cplx_inv (int *i, int *q, int norm);

dds_t* dds_create (int fs, int fc);
void dds_destroy (dds_t* instance);
void dds_reset (dds_t* instance);
void dds_next_sample (dds_t* instance, int *i, int *q);

int fir_normf (int tap[], int taps);
int fir_work (int buf[], int tap[], int taps, int dec);
int fir_ntaps (int n, int bf);
int* fir_create_taps (int n, int bf);

uint8_t crc8 (uint8_t byte[], int len);
uint16_t crc16 (uint8_t byte[], int len);

#endif
