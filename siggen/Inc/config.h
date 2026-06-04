#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdint.h>   // uint
#include "config_def.h"    // fs
#include "fs_broker.h"    // fs


int config_save (config_t* instance, fs_broker_t *fs, int fd);
int config_load (config_t* instance, fs_broker_t *fs, int fd);


#endif
