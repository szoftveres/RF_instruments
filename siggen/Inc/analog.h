#ifndef __ANALOG_H__
#define __ANALOG_H__

#include <stdint.h> // uint32_t


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



int ofdm_cplx_encode_u8 (uint8_t c, int pilot_i, int pilot_q, int *i_out, int *q_out, int samples, int dynamic_range);
uint8_t ofdm_cplx_decode_u8 (int *i_in, int *q_in, int *pilot_i, int *pilot_q, int samples);
void cplx_upconvert (dds_t* dds, int *i, int *q, int* wave, int samples);
void cplx_downconvert (dds_t* dds, int* wave, int *i, int *q, int samples);
int halfband (int *i, int *q, int samples);



dds_t* dds_create (int fs, int fc);
void dds_destroy (dds_t* instance);
void dds_reset (dds_t* instance);
void dds_next_sample (dds_t* instance, int *i, int *q);





#endif
