#include "program.h"
#include "hal_plat.h" // malloc free
#include <string.h> // strcpy memset


#define PROGRAM_SIGNATURE (0x71077345)


uint32_t program_checksum (struct program_header_s* header) {
	uint32_t chksum = 0x00;

	for (int i = 0; i != sizeof(header->fields); i++) {
		chksum += header->fields.chkbyte[i];
	}
	return chksum;
}


void validate_program (program_t* instance) {
	instance->header.fields.signature = PROGRAM_SIGNATURE;
	instance->header.checksum = program_checksum(&(instance->header));
}


int verify_program (program_t* instance, struct program_header_s* header) {
	if (header->fields.signature != PROGRAM_SIGNATURE) {
		return 0;
	}
	if (header->checksum != program_checksum(header)) {
		return 0;
	}
	if (instance->header.fields.linelen != header->fields.linelen) {
		return 0;
	}
	if (instance->header.fields.nlines != header->fields.nlines) {
		return 0;
	}
	return 1;
}


program_t *program_create (int nlines, int linelen) {
	int bail = 0;
	program_t *instance = (program_t*)t_malloc(sizeof(program_t));
	if (!instance) {
		return instance;
	}
	instance->header.fields.nlines = nlines;
	instance->header.fields.linelen = linelen;

	instance->line = (char**)t_malloc(sizeof(char*) * instance->header.fields.nlines);  // an array of char*
	if (!instance->line) {
		t_free(instance);
		instance = NULL;
		return instance;
	}
	memset(instance->line, 0x00, sizeof(char*) * instance->header.fields.nlines);

	for (int i = 0; i != instance->header.fields.nlines; i++) {
		instance->line[i] = (char*)t_malloc(instance->header.fields.linelen); // allocating each line
		if (!instance->line[i]) {
			bail = 1;
			break;
		}
		strcpy(instance->line[i], "");
	}


	if (bail) {
		for (int i = 0; i != instance->header.fields.nlines; i++) {
			if (instance->line[i]) {
				t_free(instance->line[i]);
				instance->line[i] = NULL;
			}
		}
		if (instance->line) {
			t_free(instance->line);
			instance->line = NULL;
		}
		t_free(instance);
		instance = NULL;
	}
	return instance;
}


void program_destroy (program_t* instance) {
	if (!instance) {
		return;
	}
	for (int i = 0; i != instance->header.fields.nlines; i++) {
		if (instance->line[i]) {
			t_free(instance->line[i]);
			instance->line[i] = NULL;
		}
	}
	if (instance->line) {
		t_free(instance->line);
		instance->line = NULL;
	}
	t_free(instance);
}


char* program_line (program_t* instance, int line) {
	return instance->line[line];
}


int program_save (program_t* instance, fs_t *fs, int fd) {
	int rc = 0;
	validate_program(instance);
	rc += fs_write(fs, fd, (char*)&(instance->header), sizeof(instance->header));
	for (int i = 0; i != instance->header.fields.nlines; i++) {
		int linelen = strlen(instance->line[i]) + 1; // + '\0'
		rc += fs_write(fs, fd, (char*)&linelen, sizeof(int));
		rc += fs_write(fs, fd, instance->line[i], linelen);
	}
	return rc;
}


int program_load (program_t* instance, fs_t *fs, int fd) {
	int rc = 0;

	struct program_header_s lcl_header;
	rc += fs_read(fs, fd, (char*)&(lcl_header), sizeof(lcl_header));

	if (!verify_program(instance, &lcl_header)) {
		return 0;
	}

	for (int i = 0; i != instance->header.fields.nlines; i++) {
		int linelen;
		rc += fs_read(fs, fd, (char*)&linelen, sizeof(int));
		rc += fs_read(fs, fd, instance->line[i], linelen);
	}

	return rc;
}



