#ifndef __FATSMALLFS_H__
#define __FATSMALLFS_H__

#include "blockdevice.h"
#include <stdint.h>


//-------------------------


typedef struct __attribute__((packed)) device_params_s {
	uint16_t	magic1;
	uint16_t	rootdir_entries;
	uint16_t	magic2;
} device_params_t;

#define FS_MAGIC1			(0xCAFE)
#define FS_MAGIC2			(0x5A5A)


#define FS_ATTRIB_EMPTY		(0x0000)
#define FS_ATTRIB_OCCUPIED	(0x0001)
#define FS_ATTRIB_NODELETE	(0x0002)


#define FS_FAT_EMPTY		(0x0000)
#define FS_FAT_RESERVED		(0xFFFE)
#define FS_FAT_END    		(0xFFFF)


#define FS_O_CREAT			(0x01)
#define FS_O_APPEND			(0x02)
#define FS_O_TRUNC			(0x04)

#define FILENAME_LEN (10)

typedef struct __attribute__((packed)) file_entry_s {
	uint16_t	start;		// first block
	uint16_t	attrib;
	uint16_t    size;
	char 		name[FILENAME_LEN]; 	// null terminated string
} file_entry_t;			// Size must be some integer fraction of the blocksize

typedef uint16_t block_t;

#define FS_MAX_FILES (1)

typedef struct filehandle_s {
	uint16_t 			rp;
	uint16_t			wp;
	int					direntry;
} filehandle_t;

typedef struct fs_s {
	device_params_t		params;
	blockdevice_t 		*device;

	block_t				recent_block;
	int					recent_block_dirty; // differs from disk content

	block_t				recent_fatblock;
	int					recent_fatblock_dirty;
	block_t*			fatblock;

	int					recent_direntry;
	int					recent_direntry_dirty;
	file_entry_t		direntry;

	filehandle_t		fp[FS_MAX_FILES];
} fs_t;




fs_t* fs_create (blockdevice_t* device);
void fs_destroy (fs_t* instance);
int fs_verify (fs_t* instance);
void fs_format (fs_t* instance, int direntries);
void fs_flush (fs_t* instance);
int fs_count_empyt_blocks (fs_t* instance);
int fs_count_empyt_direntries (fs_t* instance);

int fs_open (fs_t* instance, char* name, int flags);
void fs_close (fs_t* instance, int fd);
int fs_delete (fs_t* instance, char* name);
file_entry_t* fs_walk_dir (fs_t* instance, int *n);
int fs_read (fs_t* instance, int fd, char* buf, int count);
int fs_write (fs_t* instance, int fd, char* buf, int count);
int fs_stats (fs_t* instance);

//void fs_dump_fat (fs_t* instance);


#endif
