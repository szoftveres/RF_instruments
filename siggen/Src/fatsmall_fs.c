#include "fatsmall_fs.h"
#include <stdlib.h> //malloc free
#include <string.h> //memcpy

#define BLOCK_VALID(b)  ((b) && ((b) != FS_FAT_END))


void fs_flush (fs_t* instance) {
	if (instance->recent_block_dirty) {
		instance->device->write_block(instance->device, instance->recent_block);
		instance->recent_block_dirty = 0;
	}
}


char* fs_get_block (fs_t* instance, block_t block) {
	if (instance->recent_block != block) {
		fs_flush(instance);
		instance->device->read_block(instance->device, block);
		instance->recent_block = block;
	}
	return instance->device->buffer;
}


int fs_verify (fs_t* instance) {
	return ((instance->params.magic1 == FS_MAGIC1) && (instance->params.magic2 == FS_MAGIC2));
}


fs_t* fs_create (blockdevice_t* device) {
	fs_t *instance = (fs_t*)malloc(sizeof(fs_t));
	if (!instance) {
		return instance;
	}
	instance->device = device;
	instance->recent_block = FS_FAT_END;
	instance->recent_block_dirty = 0;

	instance->recent_fatblock = FS_FAT_END;
	instance->recent_fatblock_dirty = 0;
	instance->fatblock = (block_t*)malloc(instance->device->blocksize);
	if (!instance->fatblock) {
		free(instance);
		instance = NULL;
		return instance;
	}

	instance->recent_direntry = -1;
	instance->recent_direntry_dirty = 0;

	for (int fd = 0; fd != FS_MAX_FILES; fd++) {
		instance->fp[fd].direntry = -1;
	}

	// Getting the params
	memcpy(&(instance->params), fs_get_block(instance, 0), sizeof(device_params_t));

	return instance;
}


void fs_destroy (fs_t* instance) {
	if (!instance) {
		return;
	}
	free(instance->fatblock);
	free(instance);
}


int fs_get_fp_for_node (fs_t* instance, int n) {
	for (int fd = 0; fd != FS_MAX_FILES; fd++) {
		if (instance->fp[fd].direntry < 0) {
			instance->fp[fd].direntry = n;
			instance->fp[fd].rp = 0;
			instance->fp[fd].wp = 0;
			return fd;
		}
	}
	return -1;
}


block_t fs_fat_start_block (fs_t* instance) {
	return (instance->params.rootdir_entries * sizeof(file_entry_t) / instance->device->blocksize);
}

int fs_fatentries_per_block (fs_t* instance) {
	return instance->device->blocksize / sizeof(block_t);
} // == 32

int fs_fatsize (fs_t* instance) {
	return (instance->device->blocks / fs_fatentries_per_block(instance));
} // == 16

block_t fs_dataarea_start_block (fs_t* instance) {
	return fs_fat_start_block(instance) + fs_fatsize(instance);
}


void fs_load_direntry (fs_t* instance, int entry) {

	if (instance->recent_direntry != entry) {
		file_entry_t* p;
		block_t block;
		if (instance->recent_direntry_dirty) {
			block = instance->recent_direntry / (instance->device->blocksize / sizeof(file_entry_t));
			fs_get_block(instance, block);
			p = (file_entry_t*) &(instance->device->buffer[((instance->recent_direntry % (instance->device->blocksize / sizeof(file_entry_t))) * sizeof(file_entry_t))]);

			memcpy(p, &(instance->direntry), sizeof(file_entry_t));
			instance->recent_block_dirty = 1;
			instance->recent_direntry_dirty = 0;
		}
		if (entry != -1) {
			block = entry / (instance->device->blocksize / sizeof(file_entry_t));
			fs_get_block(instance, block);
			p = (file_entry_t*) &(instance->device->buffer[((entry % (instance->device->blocksize / sizeof(file_entry_t))) * sizeof(file_entry_t))]);
			memcpy(&(instance->direntry), p, sizeof(file_entry_t));
			instance->recent_direntry = entry;
		}
	}
}


// Load the fat for block
block_t* fs_load_fatblock (fs_t* instance, block_t block) {
	block_t* fatentry;

	block_t new_fatblock = fs_fat_start_block(instance) + (block / fs_fatentries_per_block(instance));

	if (instance->recent_fatblock != new_fatblock) {
		char* p;

		if (instance->recent_fatblock_dirty) {

			p = fs_get_block(instance, instance->recent_fatblock);
			memcpy(p, instance->fatblock, instance->device->blocksize);
			instance->recent_block_dirty = 1;
			instance->recent_fatblock_dirty = 0;
		}
		if (block != FS_FAT_END) { // FS_FAT_END indicates FAT flush request
			p = fs_get_block(instance, new_fatblock);
			memcpy(instance->fatblock, p, instance->device->blocksize);

			instance->recent_fatblock = new_fatblock;
		}
	}
	fatentry = instance->fatblock;

	fatentry += (block % fs_fatentries_per_block(instance));

	return fatentry;
}


void fs_flush_fat (fs_t* instance) {
	fs_load_fatblock(instance, FS_FAT_END);
	fs_load_direntry(instance, -1);
	fs_flush(instance);
}


void fs_mark_fat (fs_t* instance, block_t block, block_t flag) {
	block_t* fatentry = fs_load_fatblock(instance, block);
	*fatentry = flag;
	instance->recent_fatblock_dirty = 1;
}


void fs_format (fs_t* instance, int direntries) {
	file_entry_t lcl_entry;

	instance->params.rootdir_entries = direntries;
	instance->params.magic1 = FS_MAGIC1;
	instance->params.magic2 = FS_MAGIC2;


	for (block_t block = 0; block != instance->device->blocks; block++) {
		fs_mark_fat(instance, block, FS_FAT_EMPTY);
	}

	// Copying the params into the first entry
	memcpy(&lcl_entry, &(instance->params), sizeof(device_params_t));
	fs_load_direntry(instance, 0);
	memcpy(&(instance->direntry),  &lcl_entry, sizeof(file_entry_t));
	instance->recent_direntry_dirty = 1;

	fs_mark_fat(instance, instance->recent_block, FS_FAT_RESERVED);

	// Setting up the rest of the direntries
	for (int i = 1; i != direntries; i++) {
		lcl_entry.attrib = FS_ATTRIB_EMPTY;
		lcl_entry.start = FS_FAT_END;
		lcl_entry.size = 0;
		strcpy(lcl_entry.name, "");

		fs_load_direntry(instance, i);
		memcpy(&(instance->direntry),  &lcl_entry, sizeof(file_entry_t));
		instance->recent_direntry_dirty = 1;

		fs_mark_fat(instance, instance->recent_block, FS_FAT_RESERVED);
	}

	// Reserving the FAT area in FAT
	for (block_t block = fs_fat_start_block(instance); block != fs_dataarea_start_block(instance); block++) {
		fs_mark_fat(instance, block, FS_FAT_RESERVED);
	}
	fs_flush_fat(instance);

}


file_entry_t* fs_walk_dir (fs_t* instance, int *n) {
	if ((*n >= instance->params.rootdir_entries) || (*n < 0)) {
		return NULL;
	}
	*n += 1;
	while (*n != instance->params.rootdir_entries) {
		fs_load_direntry(instance, *n);
		if (instance->direntry.attrib) {
			return &(instance->direntry);
		}
		*n = *n + 1;
	}
	return NULL;
}


int fs_locate_direntry (fs_t* instance, char* name) {
	int n;
	for (n = 1; n != instance->params.rootdir_entries; n++) {

		fs_load_direntry(instance, n);
		if (!instance->direntry.attrib) {
			continue;
		}
		if (!strcmp(instance->direntry.name, name)) {
			return 1;
		}
	}
	return 0;
}


block_t fs_fat_find_empyt_block (fs_t* instance) {
	block_t* fat;
	block_t block;

	for (block = fs_dataarea_start_block(instance); block != instance->device->blocks; block++) {
		fat = fs_load_fatblock(instance, block);
		if (!*fat) {
			return block;
		}
	}
    return FS_FAT_END;
}


int fs_count_empyt_blocks (fs_t* instance) {
	block_t* fat;
	block_t block;
	int n = 0;

	for (block = 0; block != instance->device->blocks; block++) {
		fat = fs_load_fatblock(instance, block);
		n += *fat ? 0 : 1;
	}
    return n;
}


int fs_count_empyt_direntries (fs_t* instance) {
	int d;
	int n = 0;

	for (d = 1; d != instance->params.rootdir_entries; d++) {
		fs_load_direntry(instance, d);
		n += instance->direntry.attrib ? 0 : 1;
	}
    return n;
}


int fs_mknod (fs_t* instance, char* name, uint16_t attrib) {
	int n;
	block_t block;

	if (fs_locate_direntry(instance, name)) {
		return 0; // Already exists
	}

	block = fs_fat_find_empyt_block(instance); // TODO check disk full;

	for (n = 1; n != instance->params.rootdir_entries; n++) {
		fs_load_direntry(instance, n);
		if (!instance->direntry.attrib) {
			break;
		}
	}
	if (n == instance->params.rootdir_entries) {
		return 0;
	}
	fs_mark_fat(instance, block, FS_FAT_END); // FAT block is still valid here so we're not going to load it again (hence won't destroy entry)

	strcpy(instance->direntry.name, name);
	instance->direntry.attrib = attrib;
	instance->direntry.start = block;
	instance->direntry.size = 0;
	instance->recent_direntry_dirty = 1;

	return 1;
}


block_t fs_fat_next_block (fs_t* instance, block_t block) {
	block_t* fat;

    fat = fs_load_fatblock(instance, block);
    return *fat;
}


block_t fs_file_pos_to_block (fs_t* instance, int fd, uint16_t pos) {
	block_t block = instance->fp[fd].start;
	while ((pos / instance->device->blocksize) && BLOCK_VALID(block)) {
		block = fs_fat_next_block(instance, block);
	    pos -= instance->device->blocksize;
	}
	return block;
}

/*
void fs_dump_fat (fs_t* instance) {
	int i;
	block_t* fat;
	for (i = 0; i != instance->device->blocks; i++) {
		fat = fs_load_fatblock(instance, i);
		if (!(i % 32)) {
			console_printf("");
		}
		console_printf_e("%02x ", *fat % 256);
	}
	console_printf("");
	console_printf("Data first : %i", fs_dataarea_start_block(instance));
	console_printf("First empty: %i", fs_fat_find_empyt_block(instance));
}
*/

void fs_reclaim_blocks (fs_t* instance, block_t block) {
	while (BLOCK_VALID(block)) {
		block_t next = fs_fat_next_block(instance, block);
		fs_mark_fat(instance, block, FS_FAT_EMPTY);
		block = next;
	}
}


void fs_truncate_current_direntry (fs_t* instance) {
	block_t block;
	block_t next;

	block = instance->direntry.start;

	if (!BLOCK_VALID(block)) {
		return; // Not supposed to happen
	}

	instance->direntry.size = 0;
	instance->recent_direntry_dirty = 1;

	next = fs_fat_next_block(instance, block);
	fs_mark_fat(instance, block, FS_FAT_END);

	fs_reclaim_blocks(instance, next);
}


int fs_open (fs_t* instance, char* name, int flags) {
	int fd;
	int exists;

	if (strlen(name) + 1 > FILENAME_LEN) {
		return -1;
	}

	// When readonly, only read only
	if (flags & FS_O_READONLY) {
		if (flags != FS_O_READONLY) {
			return -1;
		}
	}

	exists = fs_locate_direntry(instance, name);

	if (!exists && (flags & FS_O_CREAT)) {
		exists = fs_mknod(instance, name, FS_ATTRIB_OCCUPIED);
	}
	if (!exists) {
		return -1;
	}
	fd = fs_get_fp_for_node(instance, instance->recent_direntry);
	if (fd < 0) {
		return fd;
	}
	instance->fp[fd].size = instance->direntry.size;
	instance->fp[fd].rp = 0;
	instance->fp[fd].wp = 0;
	instance->fp[fd].start = instance->direntry.start; // Trunc will not change this once assigned
	instance->fp[fd].flags = flags;
	if (flags & FS_O_TRUNC) {
		fs_truncate_current_direntry(instance);
		instance->fp[fd].size = 0;
	}
	//if (flags & FS_O_APPEND) {
		instance->fp[fd].wp = instance->fp[fd].size;
	//}
	return fd;
}


void fs_sync_open_direntry (fs_t* instance, int fd) {
	fs_load_direntry(instance, instance->fp[fd].direntry);
	if (instance->fp[fd].size != instance->direntry.size) {
		instance->direntry.size = instance->fp[fd].size;
		instance->recent_direntry_dirty = 1;
	}
}


void fs_close (fs_t* instance, int fd) {
	fs_sync_open_direntry(instance, fd);
	fs_flush_fat(instance);

	instance->fp[fd].direntry = -1;
}


int fs_delete (fs_t* instance, char* name) {
	int exists;
	uint16_t block;

	if (strlen(name) + 1 > FILENAME_LEN) {
		return -1;
	}

	exists = fs_locate_direntry(instance, name);

	if (!exists) {
		return -1;
	}

	block = instance->direntry.start;
	if (!BLOCK_VALID(block)) {
		return -1; // Not supposed to happen
	}

	instance->direntry.start = FS_FAT_END;
	instance->direntry.attrib = FS_ATTRIB_EMPTY;
	instance->recent_direntry_dirty = 1;

	fs_reclaim_blocks(instance, block);
	fs_flush_fat(instance);
	return 0;
}


int fs_read__ (fs_t* instance, int fd, char* buf, int count) {
	block_t block;
	uint16_t pos_in_block = instance->fp[fd].rp % instance->device->blocksize;
	uint16_t bytes = instance->device->blocksize - pos_in_block; // available bytes in the block at rp

	if ((fd < 0) || (instance->fp[fd].direntry < 0)) {
		return -1;
	}

	if (count < bytes) {
		bytes = count;
	}

	if ((instance->fp[fd].rp + bytes) > instance->fp[fd].size) {  // end of file check
		bytes = (instance->fp[fd].size % instance->device->blocksize) - pos_in_block;
	}
	if (bytes < 1) {
		return bytes;
	}

	block = fs_file_pos_to_block(instance, fd, instance->fp[fd].rp);

	if (!BLOCK_VALID(block)) {
		return -1; // Not supposed to happen
	}

	memcpy(buf, (fs_get_block(instance, block) + pos_in_block), bytes);

	instance->fp[fd].rp += bytes;

	return bytes;
}



int fs_write__ (fs_t* instance, int fd, char* buf, int count) {
	block_t block;
	int reserve = 0;
	uint16_t pos_in_block = instance->fp[fd].wp % instance->device->blocksize;
	uint16_t bytes = instance->device->blocksize - pos_in_block; // available bytes in the block at wp

	if (instance->fp[fd].flags & FS_O_READONLY) {
		return -1;
	}

	if ((fd < 0) || (instance->fp[fd].direntry < 0)) {
		return -1;
	}

	if (count < bytes) {
		bytes = count;
	}

	if (bytes < 1) {
		return bytes;
	}

	if (!((instance->fp[fd].wp + bytes) % instance->device->blocksize)) {
		reserve = 1; // Reserve new block at the end
	}

	instance->fp[fd].size += bytes; // TODO undo if the operation falls through

	block = fs_file_pos_to_block(instance, fd, instance->fp[fd].wp);

	if (!BLOCK_VALID(block)) {
		return -1; // Not supposed to happen
	}

	memcpy((fs_get_block(instance, block) + pos_in_block), buf, bytes);
	instance->recent_block_dirty = 1;

	instance->fp[fd].wp += bytes;

	if (reserve) { // TODO no append case, do we really need a new block?
		block_t newblock = fs_fat_find_empyt_block(instance); // TODO check disk full + TODO this writes FAT back to disk even if the same ends up being loaded again
		fs_mark_fat(instance, block, newblock);
		fs_mark_fat(instance, newblock, FS_FAT_END);
	}

	return bytes;
}


int fs_read (fs_t* instance, int fd, char* buf, int count) {
	int b = 0;
	while (count) {
		int rc = fs_read__(instance, fd, buf, count);

		if (rc < 1) {
			break;
		}
		b += rc;
		count -= rc;
		buf += rc;
	}
	return b;
}


int fs_write (fs_t* instance, int fd, char* buf, int count) {
	int b = 0;
	while (count) {
		int rc = fs_write__(instance, fd, buf, count);
		if (rc < 1) {
			break;
		}
		b += rc;
		count -= rc;
		buf += rc;
	}
	return b;
}
