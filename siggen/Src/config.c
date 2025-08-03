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



int config_save (config_t* instance, blockfile_t *file) {
	int rc = 0;
	int bufsize = blockfile_getbufsize(file);
	char* buf = blockfile_getbuf(file);

	int block = 0;
	int ip = 0;

	validate_config(instance);

	for (int i = 0; i != sizeof(config_t); i++) {
		buf[ip++] = instance->byte[i];
		if (ip >= bufsize) {
			rc += blockfile_write(file, block++); // buf is full, flush
			ip = 0;
		}
	}
	if (ip) {
		rc += blockfile_write(file, block++); // flush the rest
	}
	return rc;
}


int config_load (config_t* instance, blockfile_t *file) {
	int rc = 0;
	int bufsize = blockfile_getbufsize(file);
	char* buf = blockfile_getbuf(file);

	int block = 0;

	rc += blockfile_read(file, block++); // Read the first block
	int ip = 0;

	for (int i = 0; i != sizeof(config_t); i++) {
		instance->byte[i] = buf[ip++];
		if (ip >= bufsize) {
			rc += blockfile_read(file, block++); // read the next block
			ip = 0;
		}
	}

	if (!verify_config(instance)) {
		rc = 0;
	}

	return rc;
}

