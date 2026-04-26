#include "fs_broker.h"
#include "hal_plat.h"  // t_malloc
#include <string.h>    // memset


void stdiostack_push (stdio_stack_t **head, int fin, int fout, int ferr) {
	stdio_stack_t *this = (stdio_stack_t *)t_malloc(sizeof(stdio_stack_t));
	this->fin = fin;
	this->fout = fout;
	this->ferr = ferr;
	this->next = *head;
	*head = this;
}

void stdiostack_pop (stdio_stack_t **head) {
	stdio_stack_t *this = *head;
	if (this) {
		*head = this->next;
		t_free(this);
	}
}

generic_file_t * generic_file_create (char* name,
										void *context,
										int (*open) (struct generic_file_s*, int),
										void (*close) (struct generic_file_s*),
										int (*read) (struct generic_file_s*, int, char*),
										int (*write) (struct generic_file_s*, int, char*) ) {

	generic_file_t *this = (generic_file_t *)t_malloc(sizeof(generic_file_t));
	if (!this) {
		return this;
	}
	strcpy(this->name, name);
	this->open = open;
	this->close = close;
	this->read = read;
	this->write = write;
	this->context = context;

	return this;
}


generic_fs_t * generic_fs_create (void) {
	generic_fs_t *this = (generic_fs_t *)t_malloc(sizeof(generic_fs_t));
	if (!this) {
		return this;
	}
	memset(this, 0x00, sizeof(generic_fs_t));
	return this;
}


int generic_fs_register_file (generic_fs_t* instance, generic_file_t* file) {
	for (int file_idx = 0; file_idx != MAX_GENERIC_FS_FILES; file_idx++) {
		if (!instance->file[file_idx]) {
			instance->file[file_idx] = file;
			return file_idx;
		}
	}
	return -1;
}


int generic_fs_reserve_filp (generic_fs_t* instance) {
	for (int fd = 0; fd != MAX_GENERIC_FS_FILP; fd++) {
		if (!instance->fp[fd].reserved) {
			instance->fp[fd].reserved = 1;
			return fd;
		}
	}
	return -1;
}


int generic_fs_open (generic_fs_t* instance, char* name, int flags) {
	int fd;
	int file;

	// When readonly, only read only
	if (flags & FS_O_READONLY) {
		if (flags != FS_O_READONLY) {
			return -1;
		}
	}

	fd = generic_fs_reserve_filp(instance);
	if (fd < 0) {
		return fd;
	}

	for (file = 0; file != MAX_GENERIC_FS_FILES; file++) {
		if (!instance->file[file]) {
			continue;
		}
		if (!strcmp(instance->file[file]->name, name)) {
			break;
		}
	}

	if (file < MAX_GENERIC_FS_FILES) {
		instance->fp[fd].file = file;
		instance->file[file]->open(instance->file[file], flags);
	} else {
		instance->fp[fd].reserved = 0;
		fd = -1;
	}

	return fd;
}


void generic_fs_close (generic_fs_t* instance, int fd) {
	if (fd < 0) {
		return;
	}
	if (!instance->fp[fd].reserved) {
		return;
	}
	int file = instance->fp[fd].file;
	instance->file[file]->close(instance->file[file]);
	instance->fp[fd].reserved = 0;
}


void generic_fs_rewind (generic_fs_t* instance, int fd) {}


int generic_fs_write (generic_fs_t* instance, int fd, void* buf, int count) {
	if (fd < 0) {
		return fd;
	}
	//if (instance->fp[fd].flags & FS_O_READONLY) {
	//	return -1;
	//}

	int file = instance->fp[fd].file;
	return instance->file[file]->write(instance->file[file], count, buf);
}


int generic_fs_read (generic_fs_t* instance, int fd, void* buf, int count) {
	if (fd < 0) {
		return fd;
	}
	int file = instance->fp[fd].file;
	return instance->file[file]->read(instance->file[file], count, buf);
}


int generic_fs_delete (generic_fs_t* instance, char* name) {return 0;}




int generic_fs_opendir (generic_fs_t* instance) {
	instance->dirp = -1;
	return 1;
}


int generic_fs_walkdir (generic_fs_t* instance, char** name, int* size) {

	do {
		instance->dirp++;
		if (instance->dirp >= MAX_GENERIC_FS_FILES) {
			return 0; // Done
		}
	} while (!(instance->file[instance->dirp]));
	*size = 0; // XXX for now
	*name = instance->file[instance->dirp]->name;

	return 1; // Continue
}


int generic_fs_closedir (generic_fs_t* instance) {return 1;}


/* ------------------------------- */


fs_broker_t* fs_broker_create (void) {
	fs_broker_t* instance = (fs_broker_t*) t_malloc(sizeof(fs_broker_t));
	if (!instance) {
		return instance;
	}
	memset(instance, 0x00, sizeof(fs_broker_t));

	for (int fd = 0; fd != MAX_FS_BROKER_FILP; fd++) {
		instance->fp[fd].reserved = 0;
	}
	instance->current_fs = 0; // The first to get registered becomes the default
	return instance;
}


void fs_broker_destroy (fs_broker_t* instance) {
	if (!instance) {
		return;
	}
	/// XXX lots of cleanup, destroying other file systems, etc... which -given the
	/// global, ever-lasting nature of this object - will probably never get implemented
	t_free(instance);
	return;
}


int fs_broker_reserve_filp (fs_broker_t* instance) {
	for (int fd = 0; fd != MAX_FS_BROKER_FILP; fd++) {
		if (!instance->fp[fd].reserved) {
			instance->fp[fd].reserved = 1;
			return fd;
		}
	}
	return -1;
}

int fs_broker_find_empty_instance_slot (fs_broker_t* instance) {
	for (int idx = 0; idx != MAX_FS_BROKER_FS; idx++) {
		if (!instance->fs_instance[idx].instance) {
			return idx;
		}
	}
	return -1;
}


int fs_broker_register_fs (fs_broker_t* instance,
						   void* fs,
						   char letter,
						   int (*open) (void*, char*, int),
						   void (*close) (void*, int),
						   void (*rewind) (void*, int),
						   int (*read) (void*, int, char*, int),
						   int (*write) (void*, int, char*, int),
						   int (*delete) (void*, char*),
						   int (*opendir) (void*),
						   int (*walkdir) (void*, char**, int*),
						   int (*closedir) (void*)) {

	int idx = fs_broker_find_empty_instance_slot(instance);
	if (idx < 0) {
		return idx;
	}
	instance->fs_instance[idx].instance = fs;
	instance->fs_instance[idx].letter = letter;
	instance->fs_instance[idx].open = open;
	instance->fs_instance[idx].close = close;
	instance->fs_instance[idx].rewind = rewind;
	instance->fs_instance[idx].read = read;
	instance->fs_instance[idx].write = write;
	instance->fs_instance[idx].delete = delete;
	instance->fs_instance[idx].opendir = opendir;
	instance->fs_instance[idx].walkdir = walkdir;
	instance->fs_instance[idx].closedir = closedir;

	return idx;
}


char get_current_fs (fs_broker_t* broker) {
	return broker->fs_instance[broker->current_fs].letter;
}


int change_current_fs (fs_broker_t* broker, char letter) {
	for (int idx = 0; idx != MAX_FS_BROKER_FS; idx++) {
		if (broker->fs_instance[idx].instance) {
			if (broker->fs_instance[idx].letter == letter) {
				broker->current_fs = idx;
				return 1;
			}
		}
	}
	return 0;
}


char* name_to_fs_instance (fs_broker_t* broker, char* name, int* idx) {
	*idx = broker->current_fs;
	if (strlen(name) < 3) {
		return name;
	}
	if (name[1] == ':') {
		for (int i = 0; i != MAX_FS_BROKER_FS; i++) {
			if (broker->fs_instance[i].letter == name[0]) {
				*idx = i;
				return &(name[2]);
			}
		}
		*idx = -1;
		return name;
	}
	return name;
}


int open_f (fs_broker_t* broker, char* name, int flags) {
	int fs;
	int fp;
	char* finalname = name_to_fs_instance(broker, name, &fs);
	if (fs < 0) {
		return -1;
	}
	fp = fs_broker_reserve_filp(broker);
	if (fp < 0) {
		return fp;
	}

	broker->fp[fp].fs_instance_filp = broker->fs_instance[fs].open(broker->fs_instance[fs].instance, finalname, flags);
	if (broker->fp[fp].fs_instance_filp < 0) {
		return -1;
	}
	broker->fp[fp].fs_instance_idx = fs;
	broker->fp[fp].reserved = 1;

	return fp;
}


void close_f (fs_broker_t* broker, int fd) {
	int fs;
	if (fd < 0) {
		return;
	}
	if (!broker->fp[fd].reserved) {
		return;
	}
	broker->fp[fd].reserved = 0;
	fs = broker->fp[fd].fs_instance_idx;
	broker->fs_instance[fs].close(broker->fs_instance[fs].instance, broker->fp[fd].fs_instance_filp);
}


void rewind_f (fs_broker_t* broker, int fd) {
	int fs;
	fs = broker->fp[fd].fs_instance_idx;
	broker->fs_instance[fs].rewind(broker->fs_instance[fs].instance, broker->fp[fd].fs_instance_filp);
}


int read_f (fs_broker_t* broker, int fd, char* buf, int count) {
	int fs = broker->fp[fd].fs_instance_idx;
	return broker->fs_instance[fs].read(broker->fs_instance[fs].instance, broker->fp[fd].fs_instance_filp, buf, count);
}


// This function won't return unless count number of bytes is read (or EOF)
int read_f_all (fs_broker_t* broker, int fd, char* buf, int count) {
	int b = 0;
	while (count) {
		int rc = read_f(broker, fd, buf, count);

		if (rc < 1) {
			break;
		}
		b += rc;
		count -= rc;
		buf += rc;
	}
	return b;
}


int write_f (fs_broker_t* broker, int fd, char* buf, int count) {
	int fs = broker->fp[fd].fs_instance_idx;
	return broker->fs_instance[fs].write(broker->fs_instance[fs].instance, broker->fp[fd].fs_instance_filp, buf, count);
}


// This function won't return unless count number of bytes is written (or EOF)
int write_f_all (fs_broker_t* broker, int fd, char* buf, int count) {
	int b = 0;
	while (count) {
		int rc = write_f(broker, fd, buf, count);
		if (rc < 1) {
			break;
		}
		b += rc;
		count -= rc;
		buf += rc;
	}
	return b;
}


int delete_f (fs_broker_t* broker, char* name) {
	int fs;
	char* finalname = name_to_fs_instance(broker, name, &fs);
	if (fs < 0) {
		return -1;
	}
	return broker->fs_instance[fs].delete(broker->fs_instance[fs].instance, finalname);
}


int opendir_f (fs_broker_t* broker) {
	int fs = broker->current_fs;
	return broker->fs_instance[fs].opendir(broker->fs_instance[fs].instance);
}


int walkdir_f (fs_broker_t* broker, char** name, int* size) {
	int fs = broker->current_fs;
	return broker->fs_instance[fs].walkdir(broker->fs_instance[fs].instance, name, size);
}


int closedir_f (fs_broker_t* broker) {
	int fs = broker->current_fs;
	return broker->fs_instance[fs].closedir(broker->fs_instance[fs].instance);
}

