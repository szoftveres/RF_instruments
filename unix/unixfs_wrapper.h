#ifndef INC_UNIXFS_WRAPPER_H_
#define INC_UNIXFS_WRAPPER_H_

#include <stdio.h>
#include "os/fs_broker.h"
#include <sys/types.h>
#include <dirent.h>


int unix_rw (ssize_t (*func)(int, const void*, size_t), int fp, void* buf, int count);

#define UNIXFSWRAPPER_MAX_FILES (2)


typedef struct unixfs_wrapper_s {
	int cluster_size_bytes;
	struct {
		int fd;
		int reserved;
		int flags;
	} fp[UNIXFSWRAPPER_MAX_FILES];
    DIR *dirp;
} unixfs_wrapper_t;

// Only one unix FS may exist in a Unix system
int unixfswrapper_init (unixfs_wrapper_t* instance);

int unixfswrapper_open (unixfs_wrapper_t* instance, char* name, int flags);
void unixfswrapper_close (unixfs_wrapper_t* instance, int fd);
void unixfswrapper_rewind (unixfs_wrapper_t* instance, int fd);
int unixfswrapper_write (unixfs_wrapper_t* instance, int fd, void* buf, int count);
int unixfswrapper_read (unixfs_wrapper_t* instance, int fd, void* buf, int count);
int unixfswrapper_delete (unixfs_wrapper_t* instance, char* name);

int unixfswrapper_opendir (unixfs_wrapper_t* instance);
int unixfswrapper_walkdir (unixfs_wrapper_t* instance, char** name, int* size);
int unixfswrapper_closedir (unixfs_wrapper_t* instance);


#endif /* INC_UNIXFS_WRAPPER_H_ */
