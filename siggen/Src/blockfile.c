#include <blockfile.h>
#include <stdlib.h> //malloc free


blockfile_t* blockfile_create (char* (*getbuf) (void), int (*getbufsize) (void), int (*write) (int), int (*read) (int))
{
	blockfile_t *instance = (blockfile_t*)malloc(sizeof(blockfile_t));
	if (!instance) {
		return instance;
	}
	instance->getbuf = getbuf;
	instance->getbufsize = getbufsize;
	instance->read = read;
	instance->write = write;

	return instance;
}


void blockfile_destroy (blockfile_t* blockfile) {
	if (!blockfile) {
		return;
	}
	free(blockfile);
}
