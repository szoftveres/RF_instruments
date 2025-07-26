#include "program.h"
#include <stdlib.h> // malloc free
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
	program_t *instance = (program_t*)malloc(sizeof(program_t));
	if (!instance) {
		return instance;
	}
	instance->header.fields.nlines = nlines;
	instance->header.fields.linelen = linelen;

	instance->line = (char**)malloc(sizeof(char*) * instance->header.fields.nlines);  // an array of char*
	if (!instance->line) {
		free(instance);
		instance = NULL;
		return instance;
	}
	memset(instance->line, 0x00, sizeof(char*) * instance->header.fields.nlines);

	for (int i = 0; i != instance->header.fields.nlines; i++) {
		instance->line[i] = (char*)malloc(instance->header.fields.linelen); // allocating each line
		if (!instance->line[i]) {
			bail = 1;
			break;
		}
		strcpy(instance->line[i], " ");
	}


	if (bail) {
		for (int i = 0; i != instance->header.fields.nlines; i++) {
			if (instance->line[i]) {
				free(instance->line[i]);
				instance->line[i] = NULL;
			}
		}
		if (instance->line) {
			free(instance->line);
			instance->line = NULL;
		}
		free(instance);
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
			free(instance->line[i]);
			instance->line[i] = NULL;
		}
	}
	if (instance->line) {
		free(instance->line);
		instance->line = NULL;
	}
	free(instance);
}


char* program_line (program_t* instance, int line) {
	return instance->line[line];
}


int program_save (program_t* instance, blockfile_t *file) {
	int rc = 0;
	int bufsize = blockfile_getbufsize(file);
	char* buf = blockfile_getbuf(file);

	int block = 0;
	int ip = 0;

	validate_program(instance);

	for (int i = 0; i != sizeof(instance->header); i++) {
		buf[ip++] = instance->header.savebyte[i];
		if (ip >= bufsize) {
			rc += blockfile_write(file, block++); // buf is full, flush
			ip = 0;
		}
	}

	for (int i = 0; i != instance->header.fields.nlines; i++) {
		char* linebyte = instance->line[i];
		for (int j = 0; j != instance->header.fields.linelen; j++) {
			buf[ip++] = linebyte[j];
			if (ip >= bufsize) {
				rc += blockfile_write(file, block++); // buf is full, flush
				ip = 0;
			}
		}
	}
	if (ip) {
		rc += blockfile_write(file, block++); // flush the rest
	}
	return rc;
}



int program_load (program_t* instance, blockfile_t *file) {
	int rc = 0;
	int bufsize = blockfile_getbufsize(file);
	char* buf = blockfile_getbuf(file);

	struct program_header_s lcl_header;

	int block = 0;

	rc += blockfile_read(file, block++); // Read the first block
	int ip = 0;

	for (int i = 0; i != sizeof(instance->header); i++) {
		lcl_header.savebyte[i] = buf[ip++];
		if (ip >= bufsize) {
			rc += blockfile_read(file, block++); // read the next block
			ip = 0;
		}
	}

	if (!verify_program(instance, &lcl_header)) {
		return 0;
	}

	for (int i = 0; i != instance->header.fields.nlines; i++) {
		char* linebyte = instance->line[i];
		for (int j = 0; j != instance->header.fields.linelen; j++) {
			linebyte[j] = buf[ip++];
			if (ip >= bufsize) {
				rc += blockfile_read(file, block++); // read the next block
				ip = 0;
			}
		}
	}

	return rc;
}


