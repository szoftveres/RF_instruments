#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdint.h>
#include "fatsmall_fs.h"


typedef struct config_s {
	uint8_t		byte[0];  // This just helps with byte-level access
	struct {
		uint8_t		byte[0];  // This just helps with byte-level access
		uint32_t 	khz;
		int			level;
		int			rfon;
		int			echoon;

		int			correction;
		uint32_t	signature;
	} fields;

	uint32_t	checksum;
} config_t;


int config_save (config_t* instance, fs_t *fs, int fd);
int config_load (config_t* instance, fs_t *fs, int fd);


#endif
