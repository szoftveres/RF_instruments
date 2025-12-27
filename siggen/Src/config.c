#include "config.h"


#define CONFIG_SIGNATURE (0x71077345)


uint32_t config_checksum (config_t* instance) {
	uint32_t chksum = 0x00;

	for (int i = 0; i != sizeof(instance->fields); i++) {
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
		return 0;
	}
	if (instance->checksum != config_checksum(instance)) {
		return 0;
	}
	return 1;
}



int config_save (config_t* instance, fs_broker_t *fs, int fd) {
	int rc = 0;

	validate_config(instance);

	rc += write_f(fs, fd, (char*)instance, sizeof(config_t));

	return rc;
}


int config_load (config_t* instance, fs_broker_t *fs, int fd) {
	int rc = 0;

	rc += read_f(fs, fd, (char*)instance, sizeof(config_t));

	if (!verify_config(instance)) {
		rc = 0;
	}

	return rc;
}

