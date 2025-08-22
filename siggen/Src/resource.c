#include "resource.h"
#include <stdlib.h> //malloc free
#include <string.h> //malloc free


static resource_t *resource_head = NULL;


resource_t* resource_add (char* name, void* context, void (*setter) (void *, int), int (*getter) (void *)) {

	resource_t* instance = (resource_t*)malloc(sizeof(resource_t));
	if (!instance) {
		return instance;
	}
	instance->context = context ? context : instance;
	instance->set = setter;
	instance->get = getter;
	instance->value = 0;
	strncpy(instance->name, name, MAX_LEN_RESOURCE_NAME);


	instance->next = resource_head;
	resource_head = instance;

	return instance;
}


void variable_setter (void * context, int value) {
	resource_t* resource = (resource_t*)context;
	resource->value = value;
}

int variable_getter (void * context) {
	resource_t* resource = (resource_t*)context;
	return resource->value;
}

void void_setter (void * context, int value) {}

resource_t* locate_resource (char* name) {
	resource_t* i;

	for (i = resource_head; i; i = i->next) {
		if (!strcmp(i->name, name)) {
			return i;
		}
	}
	return i;
}

