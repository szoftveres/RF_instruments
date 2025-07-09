#include "resource.h"
#include <stdlib.h> //malloc free
#include <string.h> //malloc free
#include "functions.h"



#define GEN_PURPOSE_VARIABLES ('z'-'a' + 1)

#define TOTAL_RESOURCES (GEN_PURPOSE_VARIABLES)

static variable_t variable['z'-'a' + 1];
static resource_t * frequency_resource;
static resource_t * rf_level_resource;

static const char* invalid_val = "Invalid value \'%i\'";


resource_t* resource_create (void* context, void (*setter) (void *, int), int (*getter) (void *)) {

	resource_t* instance = (resource_t*)malloc(sizeof(resource_t));
	if (!instance) {
		return instance;
	}
	instance->context = context;
	instance->set = setter;
	instance->get = getter;
	return instance;
}


void resource_destroy (resource_t* instance) {
	if (!instance) {
		return;
	}
	free(instance);
}


void variable_setter (void * context, int value) {
	variable_t* variable = (variable_t*)context;
	variable->value = value;
}

int variable_getter (void * context) {
	variable_t* variable = (variable_t*)context;
	return variable->value;
}


void frequency_setter (void * context, int khz) {
	double actual = set_rf_frequency(khz);
	if (actual < 0) {
		console_printf(invalid_val, khz);
		return;
	}
	console_printf("actual: %i.%03i kHz", (int)actual, (int)(actual * 1000.0) % 1000);
	print_cfg();
}

int frequency_getter (void * context) {
	return config.fields.khz;
}

void rflevel_setter (void * context, int dBm) {
	if (!set_rf_level(dBm)) {
		console_printf(invalid_val, dBm);
		return;
	}
	print_cfg();
}

int rflevel_getter (void * context) {
	return config.fields.level;
}

void resources_setup (void) {
	for (int i = 0; i != (sizeof(variable)/sizeof(variable[0])); i++) {
		variable[i].resource = resource_create(&variable[i], variable_setter, variable_getter);
		// TODO malloc error check
	}
	frequency_resource = resource_create(NULL, frequency_setter, frequency_getter);
	rf_level_resource = resource_create(NULL, rflevel_setter, rflevel_getter);
}


resource_t* locate_resource (char* identifier) {
	char lcl_var_name[] = {'a', '\0'};

	for (int i = 0; i != (sizeof(variable)/sizeof(variable[0])); i++) {
		lcl_var_name[0] = 'a' + i;
		if (!strcmp(lcl_var_name, identifier)) {
			return variable[i].resource;
		}
	}

	if (!strcmp("freq", identifier)) {
		return frequency_resource;
	} else if (!strcmp("level", identifier)) {
		return rf_level_resource;
	}

	return NULL;

}

