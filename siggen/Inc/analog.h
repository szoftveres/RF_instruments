#ifndef __ANALOG_H__
#define __ANALOG_H__


int dac_on (int fs, int fc, int buflen);
void dac_off (void);

int adc_on (int fs, int fc, int buflen);
void adc_off (void);

int* dac_get_buf (void);
int* adc_get_buf (void);

void dds_next_sample (int *i, int *q);


#endif
