#ifndef INC_SDFATFS_WRAPPER_H_
#define INC_SDFATFS_WRAPPER_H_

#include <fatfs.h>
#include "../os/fs_broker.h"

int sd_rw (FRESULT (*func)(FIL*, const void*, UINT, UINT*), FIL* fp, void* buf, int count);

#define SDFATFSWRAPPER_MAX_FILES (2)


typedef struct sdfatfs_wrapper_s {
	int cluster_size_bytes;
	struct {
		FIL fil;
		int reserved;
		char* w_buffer;
		int wp;
		char* r_buffer;
		int rp;
		int rx;
		int flags;
	} fp[SDFATFSWRAPPER_MAX_FILES];
	DIR dp;
	FILINFO fno;
} sdfatfs_wrapper_t;

// Apparently only one SDcard FS can exist on an STM32 system
int sdfatfswrapper_init (sdfatfs_wrapper_t* instance);

int sdfatfswrapper_open (sdfatfs_wrapper_t* instance, char* name, int flags);
void sdfatfswrapper_close (sdfatfs_wrapper_t* instance, int fd);
void sdfatfswrapper_rewind (sdfatfs_wrapper_t* instance, int fd);
int sdfatfswrapper_write (sdfatfs_wrapper_t* instance, int fd, void* buf, int count);
int sdfatfswrapper_read (sdfatfs_wrapper_t* instance, int fd, void* buf, int count);
int sdfatfswrapper_delete (sdfatfs_wrapper_t* instance, char* name);

int sdfatfswrapper_opendir (sdfatfs_wrapper_t* instance);
int sdfatfswrapper_walkdir (sdfatfs_wrapper_t* instance, char** name, int* size);
int sdfatfswrapper_closedir (sdfatfs_wrapper_t* instance);


#endif /* INC_SDFATFS_WRAPPER_H_ */
