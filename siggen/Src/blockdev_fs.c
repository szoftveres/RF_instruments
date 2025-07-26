#include "blockdev_fs.h"
#include <stdlib.h> //malloc free


blockfile_t* blockfile_create (blockdevice_t* device, int startblock) {
	blockfile_t *instance = (blockfile_t*)malloc(sizeof(blockfile_t));
	if (!instance) {
		return instance;
	}
	instance->device = device;
	instance->startblock = startblock;

	return instance;
}


void blockfile_destroy (blockfile_t* blockfile) {
	if (!blockfile) {
		return;
	}
	free(blockfile);
}



char* blockfile_getbuf (blockfile_t* file) {
	return file->device->buffer;
}

int blockfile_getbufsize (blockfile_t* file) {
	return file->device->blocksize;
}

int blockfile_write (blockfile_t* file, int blockaddr) {
	return file->device->write_block(file->device, file->startblock + blockaddr);
}

int blockfile_read (blockfile_t* file, int blockaddr) {
	return file->device->read_block(file->device, file->startblock + blockaddr);
}
