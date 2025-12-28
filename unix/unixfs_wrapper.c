#include "unixfs_wrapper.h"
#include "../os/hal_plat.h" // malloc
#include <string.h> //memcpy

#include <fcntl.h>
#include <unistd.h>


int unix_rw (ssize_t (*func)(int, const void*, size_t), int fp, void* buf, int count) {
	int b = 0;
	ssize_t bytesProcessed;

	while (count) {
		bytesProcessed = func(fp, buf, count);
		if (bytesProcessed < 1) {
			break;
		}
		b += (int)bytesProcessed;
		count -= (int)bytesProcessed;
		buf += (int)bytesProcessed;
	}
	return b;
}


int unixfswrapper_reserve_filp (unixfs_wrapper_t* instance) {
	for (int fd = 0; fd != UNIXFSWRAPPER_MAX_FILES; fd++) {
		if (!instance->fp[fd].reserved) {
			instance->fp[fd].reserved = 1;
			return fd;
		}
	}
	return -1;
}


int unixfswrapper_init (unixfs_wrapper_t* instance) {
	if (!instance) {
		return 0;
	}
	memset(instance, 0x00, sizeof(unixfs_wrapper_t));

	for (int fd = 0; fd != UNIXFSWRAPPER_MAX_FILES; fd++) {
		instance->fp[fd].reserved = 0;
	}

	return 1;
}


int unixfswrapper_open (unixfs_wrapper_t* instance, char* name, int flags) {
	int fd;
    int mode;

	// When readonly, only read only
	if (flags & FS_O_READONLY) {
		if (flags != FS_O_READONLY) {
			return -1;
		}
        mode = O_RDONLY;
	} else {
		mode = O_RDWR;
	}
    if (flags & FS_O_APPEND) {
        mode |= O_APPEND;
    }
	if (flags & FS_O_CREAT) {
		mode |= O_CREAT;
	}
	if (flags & FS_O_TRUNC) {
		mode |= O_TRUNC;
	}

	fd = unixfswrapper_reserve_filp(instance);
	if (fd < 0) {
		return fd;
	}

	instance->fp[fd].fd = open(name, mode);
	if (instance->fp[fd].fd < 0) {
		instance->fp[fd].reserved = 0;
		return -1;
	}
	instance->fp[fd].flags = flags;

	return fd;
}


void unixfswrapper_close (unixfs_wrapper_t* instance, int fd) {
	if (fd < 0) {
		return fd;
	}
	if (!instance->fp[fd].reserved) {
		return;
	}
	close(instance->fp[fd].fd);
	instance->fp[fd].reserved = 0;
}


void unixfswrapper_rewind (unixfs_wrapper_t* instance, int fd) {
	if (fd < 0) {
		return fd;
	}
    lseek(instance->fp[fd].fd, 0, SEEK_SET);
}


int unixfswrapper_write (unixfs_wrapper_t* instance, int fd, void* buf, int count) {
	if (fd < 0) {
		return fd;
	}
	if (instance->fp[fd].flags & FS_O_READONLY) {
		return -1;
	}
	return unix_rw(write, instance->fp[fd].fd, buf, count);
}


int unixfswrapper_read (unixfs_wrapper_t* instance, int fd, void* buf, int count) {
	if (fd < 0) {
		return fd;
	}
	return unix_rw(read, instance->fp[fd].fd, buf, count);
}


int unixfswrapper_delete (unixfs_wrapper_t* instance, char* name) {
	if (unlink(name)) {
		return -1;
	}
	return 0;
}


int unixfswrapper_opendir (unixfs_wrapper_t* instance) {
	instance->dirp = opendir(".");
	if (!instance->dirp) {
		return 0;
	}
	return 1;
}


int unixfswrapper_walkdir (unixfs_wrapper_t* instance, char** name, int* size) {
    struct dirent *dirent;

	do {
		dirent = readdir(instance->dirp);
		if (!dirent) {
			return 0; // Done
		}
	} while (!(dirent->d_type == DT_REG)); // For now, skip everything that's not a simple regular file
	*size = 0; // XXX for now
	*name = dirent->d_name;
	return 1; // Continue
}


int unixfswrapper_closedir (unixfs_wrapper_t* instance) {
	if (closedir(instance->dirp)) {
		return 0;
	}
	return 1;
}

/*


file_entry_t* fs_walk_dir (fs_t* instance, int *n);

int fs_stats (fs_t* instance);

*/
