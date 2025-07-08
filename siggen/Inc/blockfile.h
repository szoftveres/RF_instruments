#ifndef __BLOCKFILE_H__
#define __BLOCKFILE_H__



typedef struct blockfile_s {
	char* (*getbuf) (void);
	int (*getbufsize) (void);
	int (*write) (int);
	int (*read) (int);

} blockfile_t;

blockfile_t* blockfile_create (char* (*getbuf) (void), int (*getbufsize) (void), int (*write) (int), int (*read) (int));

void blockfile_destroy (blockfile_t* blockfile);

#endif
