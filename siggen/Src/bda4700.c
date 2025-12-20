#include <bda4700.h>
#include <stdlib.h> // malloc


bda4700_t* bda4700_create (void (*writer) (struct bda4700_s*, uint8_t)) {

	bda4700_t* instance = (bda4700_t*) malloc(sizeof(bda4700_t));
	if (!instance) {
		return instance;
	}
	instance->writer = writer;
	// LE pin high
	bda4700_set(instance, 30.0f);

	return instance;
}


void bda4700_destroy (bda4700_t* instance) {
	if (!instance) {
		return;
	}
	free(instance);
}


int bda4700_set (bda4700_t* instance, float dB) {
	uint8_t n = 0;
	uint8_t dout = 0;

	if (dB < 0.0 || dB > 31.5) {
		return 0;
	}
	for (float atten = 0.0f; atten < dB; atten += 0.25) {
		n += 1;
	}

	/* Turning the bits around (LSB first) */
	for (int i = 0; i != 8; i++) {
		dout = dout << 1;
		dout |= (n & 0x01);
		n = n >> 1;
	}
	instance->writer(instance, dout);
	return 1;
}

