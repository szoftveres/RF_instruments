#ifndef __BLOCKDEVICE_H__
#define __BLOCKDEVICE_H__



typedef struct blockdevice_s {
	int		blocksize;
	int 	blocks;
	char*	buffer; // it must exist somewhere, so why not here?
	void*	context;
	int 	(*read_block) (struct blockdevice_s*, int);
	int 	(*write_block) (struct blockdevice_s*, int);
} blockdevice_t;


blockdevice_t* blockdevice_create (int blocksize, int blocks, int (*reader) (struct blockdevice_s*, int), int (*writer) (struct blockdevice_s*, int), void *context);

void* blockdevice_destroy (blockdevice_t* blockdevice);


#endif
