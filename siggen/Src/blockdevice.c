#include "blockdevice.h"
#include "hal_plat.h" //malloc free


blockdevice_t* blockdevice_create (int blocksize, int blocks, int (*reader) (struct blockdevice_s*, int), int (*writer) (struct blockdevice_s*, int)) {
	blockdevice_t *instance = (blockdevice_t*)t_malloc(sizeof(blockdevice_t));
	if (!instance) {
		return instance;
	}
	instance->blocksize = blocksize;
	instance->blocks = blocks;
	instance->read_block = reader;
	instance->write_block = writer;
	instance->buffer = (char*)t_malloc(instance->blocksize);
	if (!instance->buffer) {
		t_free(instance);
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
		t_free(blockdevice->buffer);
	}
	t_free(blockdevice);
}

