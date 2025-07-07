#include "config.h"


#define CONFIG_SIGNATURE (0x71077345)


uint32_t config_checksum (config_t* instance) {
	uint32_t chksum = 0x00;

	for (int i = 0; i != sizeof(struct config_fields); i++) {
		chksum += instance->fields.byte[i];
	}
	return chksum;
}


void validate_config (config_t* instance) {
	instance->fields.signature = CONFIG_SIGNATURE;
	instance->checksum = config_checksum(instance);
}


int verify_config (config_t* instance) {
	if (instance->fields.signature != CONFIG_SIGNATURE) {
		return 1;
	}
	if (instance->checksum != config_checksum(instance)) {
		return 1;
	}
	return 0;
}
