#include "sdfatfs_wrapper.h"
#include "../os/hal_plat.h" // malloc
#include <string.h> //memcpy


#define WB_SIZE (instance->cluster_size_bytes)
#define RB_SIZE (instance->cluster_size_bytes / 16)


int sd_rw (FRESULT (*func)(FIL*, const void*, UINT, UINT*), FIL* fp, void* buf, int count) {
	int b = 0;
	FRESULT fres;
	unsigned int bytesProcessed;

	while (count) {
		fres = func(fp, buf, count, &bytesProcessed);
		if (fres != FR_OK) {
			return 0;
		}
		if (bytesProcessed < 1) {
			break;
		}
		b += bytesProcessed;
		count -= bytesProcessed;
		buf += bytesProcessed;
	}
	return b;
}


int sdfatfswrapper_reserve_filp (sdfatfs_wrapper_t* instance) {
	for (int fd = 0; fd != SDFATFSWRAPPER_MAX_FILES; fd++) {
		if (!instance->fp[fd].reserved) {
			instance->fp[fd].reserved = 1;
			return fd;
		}
	}
	return -1;
}


int sdfatfswrapper_init (sdfatfs_wrapper_t* instance) {
	FRESULT fres;
	FATFS* getFreeFs;
	DWORD free_clusters;

	if (!instance) {
		return 0;
	}
	memset(instance, 0x00, sizeof(sdfatfs_wrapper_t));

	for (int fd = 0; fd != SDFATFSWRAPPER_MAX_FILES; fd++) {
		instance->fp[fd].reserved = 0;
	}

	// mount the default drive
	fres = f_mount(&SDFatFS, "", 0);
	if (fres != FR_OK) {
		t_free(instance);
		return 0;
	}

	fres = f_getfree("", &free_clusters, &getFreeFs);
	if (fres != FR_OK) {
		t_free(instance);
		return 0;
	}
	instance->cluster_size_bytes = getFreeFs->csize * 512;

	return 1;
}


int sdfatfswrapper_open (sdfatfs_wrapper_t* instance, char* name, int flags) {
	int fd;
	FRESULT fres;
	BYTE mode = FA_READ;

	// When readonly, only read only
	if (flags & FS_O_READONLY) {
		if (flags != FS_O_READONLY) {
			return -1;
		}
	} else {
		mode |= FA_WRITE;
	}

	if (flags & FS_O_CREAT) {
		mode |= FA_OPEN_ALWAYS;
	}
	if (flags & FS_O_TRUNC) {
		mode |= FA_CREATE_ALWAYS;
	}

	fd = sdfatfswrapper_reserve_filp(instance);
	if (fd < 0) {
		return fd;
	}

	fres = f_open(&(instance->fp[fd].fil), name, mode);
	if (fres != FR_OK) {
		instance->fp[fd].reserved = 0;
		return -1;
	}
	instance->fp[fd].flags = flags;
	instance->fp[fd].w_buffer = (char*)t_malloc(WB_SIZE);
	instance->fp[fd].wp = 0;
	if (!(instance->fp[fd].w_buffer)) {
		instance->fp[fd].reserved = 0;
		return -1;
	}
	instance->fp[fd].r_buffer = (char*)t_malloc(RB_SIZE);
	instance->fp[fd].rp = 0;
	instance->fp[fd].rx = 0;
	if (!(instance->fp[fd].r_buffer)) {
		t_free(instance->fp[fd].w_buffer);
		instance->fp[fd].reserved = 0;
		return -1;
	}

	return fd;
}


void sdfatfswrapper_flush (sdfatfs_wrapper_t* instance, int fd) {
	if (instance->fp[fd].wp) {
		sd_rw(f_write, &(instance->fp[fd].fil), instance->fp[fd].w_buffer, instance->fp[fd].wp);
	}
	instance->fp[fd].wp = 0;
	instance->fp[fd].rp = 0;
	instance->fp[fd].rx = 0;
}


void sdfatfswrapper_close (sdfatfs_wrapper_t* instance, int fd) {
	if (!instance->fp[fd].reserved) {
		return;
	}
	sdfatfswrapper_flush(instance, fd);
	t_free(instance->fp[fd].w_buffer);
	t_free(instance->fp[fd].r_buffer);
	f_close(&(instance->fp[fd].fil));
	instance->fp[fd].reserved = 0;
}


void sdfatfswrapper_rewind (sdfatfs_wrapper_t* instance, int fd) {
	sdfatfswrapper_flush(instance, fd);
	f_rewind(&(instance->fp[fd].fil));
}


int sdfatfswrapper_write (sdfatfs_wrapper_t* instance, int fd, void* buf, int count) {
	int bc = 0;

	if (fd < 0) {
		return fd;
	}

	if (instance->fp[fd].flags & FS_O_READONLY) {
		return -1;
	}

	while ((instance->fp[fd].wp + count) > WB_SIZE) {
		int diff_bytes = WB_SIZE - instance->fp[fd].wp;
		memcpy(&(instance->fp[fd].w_buffer[instance->fp[fd].wp]), buf, diff_bytes);
		sd_rw(f_write, &(instance->fp[fd].fil), instance->fp[fd].w_buffer, WB_SIZE);
		buf += diff_bytes;
		count -= diff_bytes;
		instance->fp[fd].wp = 0;
		bc += diff_bytes;
	}
	if (count) {
		memcpy(&(instance->fp[fd].w_buffer[instance->fp[fd].wp]), buf, count);
		instance->fp[fd].wp += count;
		bc += count;
	}
	return bc;
}


int sdfatfswrapper_read (sdfatfs_wrapper_t* instance, int fd, void* buf, int count) {
	if (fd < 0) {
		return fd;
	}

	if ((instance->fp[fd].rp + count) > instance->fp[fd].rx) {
		instance->fp[fd].rx = sd_rw((FRESULT (*)(FIL*, const void*, UINT, UINT*))f_read,
				                    &(instance->fp[fd].fil),
									instance->fp[fd].r_buffer,
									RB_SIZE); // XXX This assumes we never read more than this at once
		instance->fp[fd].rp = 0;
		if (count > instance->fp[fd].rx) {
			count = instance->fp[fd].rx;
		}
	}

	memcpy(buf, &(instance->fp[fd].r_buffer[instance->fp[fd].rp]), count);
	instance->fp[fd].rp += count;
	return count;

	// Typecast allows the use of common rw function without compiler warning
	//return sd_rw((FRESULT (*)(FIL*, const void*, UINT, UINT*))f_read, &(instance->fp[fd].fil), buf, count);
}


int sdfatfswrapper_delete (sdfatfs_wrapper_t* instance, char* name) {
	FRESULT fres;
	fres = f_unlink(name);
	if (fres != FR_OK) {
		return -1;
	}
	return 0;
}


int sdfatfswrapper_opendir (sdfatfs_wrapper_t* instance) {
	FRESULT fres;
	fres = f_opendir(&(instance->dp), "/");
	if (fres != FR_OK) {
		return 0;
	}
	return 1;
}


int sdfatfswrapper_walkdir (sdfatfs_wrapper_t* instance, char** name, int* size) {
	FRESULT fres;

	do {
		fres = f_readdir(&(instance->dp), &(instance->fno));
		if (fres != FR_OK) {
			return 0; // Error reading
		}
		if (!(instance->fno.fname[0])) {
			return 0; // End
		}
	} while (!((instance->fno.fattrib) & 0x20)); // For now, skip everything that's not 'ARCHIVE' (i.e. not a simple regular file)
	*size = instance->fno.fsize;
	*name = instance->fno.fname;
	return 1; // Continue
}


int sdfatfswrapper_closedir (sdfatfs_wrapper_t* instance) {
	FRESULT fres;
	fres = f_closedir(&(instance->dp));
	if (fres != FR_OK) {
		return 0;
	}
	return 1;
}

/*


file_entry_t* fs_walk_dir (fs_t* instance, int *n);

int fs_stats (fs_t* instance);

*/
