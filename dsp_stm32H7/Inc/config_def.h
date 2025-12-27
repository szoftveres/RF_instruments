#ifndef __CONFIG_DEF_H__
#define __CONFIG_DEF_H__

#include <stdint.h>   // uint


typedef struct config_s {
	uint8_t		byte[0];  // This just helps with byte-level access
	struct {
		uint8_t		byte[0];  // This just helps with byte-level access
		uint32_t 	khz;
		int			level;
		int			rfon;
		int			fs; 		// sampling freq
		int			fc;			// baseband carrier freq

		int			correction;
		uint32_t	signature;
	} fields;

	uint32_t	checksum;
} config_t;


#endif
