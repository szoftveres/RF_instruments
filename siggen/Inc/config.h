#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdint.h>

typedef struct config_s {
	struct config_fields {
		uint8_t		byte[0];
		uint32_t 	khz;
		int			level;
		int			rfon;


		int			correction;
		uint32_t	signature;
	} fields;

	uint32_t	checksum;
} config_t;

void validate_config (config_t* config);
int verify_config (config_t* config);

#endif
