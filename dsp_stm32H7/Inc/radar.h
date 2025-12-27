#ifndef __RADAR_H__
#define __RADAR_H__


typedef enum {
	NONE,
	DIFFERENTIAL,
	PRE_SAMPLE,
} clutter_removal_t;


int radar_func (int cycles, int fd, clutter_removal_t clutter_removal);

#endif
