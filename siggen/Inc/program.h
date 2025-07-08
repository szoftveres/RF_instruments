#ifndef __PROGRAM_H__
#define __PROGRAM_H__

#include <stdint.h>
#include "blockfile.h"

typedef struct program_s {
	struct {
		uint8_t		savebyte[0];  // This just helps with byte-level access
		struct {
			uint8_t		chkbyte[0];  // This just helps with byte-level access
			int 		nlines;
			int			linelen;
			uint32_t	signature;
		} fields;
		uint32_t	checksum; // We save the checksum but don't include it in the verification
	} saved_fields;
	char **line; // an array of char* // we don't save this; it must exist all the time
} program_t;




program_t *program_create (int nlines, int linelen);
void program_destroy (program_t* instance);
int program_binary_size (program_t* instance);
char* program_line (program_t* instance, int line);

int program_save (program_t* instance, blockfile_t *file);
int program_load (program_t* instance, blockfile_t *file);

#endif
