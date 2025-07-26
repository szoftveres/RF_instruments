#ifndef __BLOCKDEV_FS_H__
#define __BLOCKDEV_FS_H__

#include "blockdevice.h"

typedef struct blockfile_s {
	blockdevice_t *device;
	int           startblock; // 1st block of the file

} blockfile_t;



typedef struct {
	char     	name[16];
	int 	 	baseaddress;
	blockfile_t *file;
} direntry_t;



blockfile_t* blockfile_create (blockdevice_t* device, int startblock);

void blockfile_destroy (blockfile_t* blockfile);


char* blockfile_getbuf (blockfile_t* file);
int blockfile_getbufsize (blockfile_t* file);
int blockfile_write (blockfile_t* file, int blockaddr);
int blockfile_read (blockfile_t* file, int blockaddr);





#endif
