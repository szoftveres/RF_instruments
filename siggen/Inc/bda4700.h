#ifndef __BDA4700__H_
#define __BDA4700__H_

#include <stdint.h>  // uin8_t


typedef struct bda4700_s {
	void (*writer) (struct bda4700_s*, uint8_t);
} bda4700_t;

bda4700_t* bda4700_create (void (*writer) (struct bda4700_s*, uint8_t));
void bda4700_destroy (bda4700_t* instance);

int bda4700_set (bda4700_t* instance, float dB);


#endif
