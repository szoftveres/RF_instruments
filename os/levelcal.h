#ifndef __LEVELCAL_H__
#define __LEVELCAL_H__

#include <stdint.h>

#define LEVELCAL_POINTS (64)   // sizeof(levelcal_t) must be less than 256

typedef struct levelcal_s {
	struct levelcal_fields {
		char	pwr[LEVELCAL_POINTS];
		uint8_t	att[LEVELCAL_POINTS];

		uint32_t	signature;
	} fields;

	uint32_t	checksum;
} levelcal_t;

void validate_levelcal (levelcal_t* levelcal);
int verify_levelcal (levelcal_t* levelcal);
int get_cal_range_index (int freq) ;

#endif
