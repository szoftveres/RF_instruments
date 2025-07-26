#include "blockdevice.h"
#include <stdlib.h> //malloc free


blockdevice_t* blockdevice_create (int blocksize, int (*reader) (struct blockdevice_s*, int), int (*writer) (struct blockdevice_s*, int)) {
	blockdevice_t *instance = (blockdevice_t*)malloc(sizeof(blockdevice_t));
	if (!instance) {
		return instance;
	}
	instance->blocksize = blocksize;
	instance->read_block = reader;
	instance->write_block = writer;
	instance->buffer = (char*)malloc(instance->blocksize);
	if (!instance->buffer) {
		free(instance);
		instance = NULL;
		return instance;
	}
	return instance;
}


void blockdevice_destroy (blockdevice_t* blockdevice) {
	if (!blockdevice) {
		return;
	}
	if (blockdevice->buffer) {
		free(blockdevice->buffer);
	}
	free(blockdevice);
}

