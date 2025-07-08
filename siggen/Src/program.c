#include "program.h"
#include <stdlib.h> // malloc free
#include <string.h> // strcpy memset


#define PROGRAM_SIGNATURE (0x71077345)


uint32_t program_checksum (program_t* instance) {
	uint32_t chksum = 0x00;

	for (int i = 0; i != sizeof(instance->saved_fields.fields); i++) {
		chksum += instance->saved_fields.fields.chkbyte[i];
	}

	for (int i = 0; i != instance->saved_fields.fields.nlines; i++) {
		char* linebyte = instance->line[i];
		for (int j = 0; j != instance->saved_fields.fields.linelen; j++) {
			chksum += linebyte[j];
		}
	}
	return chksum;
}


void validate_program (program_t* instance) {
	instance->saved_fields.fields.signature = PROGRAM_SIGNATURE;
	instance->saved_fields.checksum = program_checksum(instance);
}


int verify_program (program_t* instance) {
	if (instance->saved_fields.fields.signature != PROGRAM_SIGNATURE) {
		return 0;
	}
	if (instance->saved_fields.checksum != program_checksum(instance)) {
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
	instance->saved_fields.fields.nlines = nlines;
	instance->saved_fields.fields.linelen = linelen;

	instance->line = (char**)malloc(sizeof(char*) * instance->saved_fields.fields.nlines);  // an array of char*
	if (!instance->line) {
		free(instance);
		instance = NULL;
		return instance;
	}
	memset(instance->line, 0x00, sizeof(char*) * instance->saved_fields.fields.nlines);

	for (int i = 0; i != instance->saved_fields.fields.nlines; i++) {
		instance->line[i] = (char*)malloc(instance->saved_fields.fields.linelen); // allocating each line
		if (!instance->line[i]) {
			bail = 1;
			break;
		}
		strcpy(instance->line[i], " ");
	}


	if (bail) {
		for (int i = 0; i != instance->saved_fields.fields.nlines; i++) {
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
	for (int i = 0; i != instance->saved_fields.fields.nlines; i++) {
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


int program_binary_size (program_t* instance) {
	return sizeof(program_t) +  (sizeof(char*) * instance->saved_fields.fields.nlines) + (instance->saved_fields.fields.linelen * instance->saved_fields.fields.nlines);
}


char* program_line (program_t* instance, int line) {
	return instance->line[line];
}


int program_save (program_t* instance, blockfile_t *file) {
	int rc = 0;
	int bufsize = file->getbufsize();
	char* buf = file->getbuf();

	int block = 0;
	int ip = 0;

	validate_program(instance);

	for (int i = 0; i != sizeof(instance->saved_fields); i++) {
		buf[ip++] = instance->saved_fields.savebyte[i];
		if (ip >= bufsize) {
			rc += file->write(block++); // buf is full, flush
			ip = 0;
		}
	}

	for (int i = 0; i != instance->saved_fields.fields.nlines; i++) {
		char* linebyte = instance->line[i];
		for (int j = 0; j != instance->saved_fields.fields.linelen; j++) {
			buf[ip++] = linebyte[j];
			if (ip >= bufsize) {
				rc += file->write(block++); // buf is full, flush
				ip = 0;
			}
		}
	}
	if (ip) {
		rc += file->write(block++); // flush the rest
	}
	return rc;
}



int program_load (program_t* instance, blockfile_t *file) {
	int rc = 0;
	int bufsize = file->getbufsize();
	char* buf = file->getbuf();

	int block = 0;

	rc += file->read(block++); // Read the first block
	int ip = 0;

	for (int i = 0; i != sizeof(instance->saved_fields); i++) {
		instance->saved_fields.savebyte[i] = buf[ip++];
		if (ip >= bufsize) {
			rc += file->read(block++); // read the next block
			ip = 0;
		}
	}

	for (int i = 0; i != instance->saved_fields.fields.nlines; i++) {
		char* linebyte = instance->line[i];
		for (int j = 0; j != instance->saved_fields.fields.linelen; j++) {
			linebyte[j] = buf[ip++];
			if (ip >= bufsize) {
				rc += file->read(block++); // read the next block
				ip = 0;
			}
		}
	}

	if (!verify_program(instance)) {
		rc = 0;
	}

	return rc;
}


