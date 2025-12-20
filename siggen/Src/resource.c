#include "resource.h"
#include <stdlib.h> //malloc free
#include <string.h> //malloc free


static resource_t *resource_head = NULL;


resource_t* resource_add (char* name, void* context, int (*setter) (void *, int), int (*getter) (void *)) {

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


resource_t* resource_it_start (void) {
	return resource_head;
}


resource_t* resource_it_next (resource_t* r) {
	if (r) {
		r = r->next;
	}
	return r;
}


int variable_setter (void * context, int value) {
	resource_t* resource = (resource_t*)context;
	resource->value = value;
	return 1;
}


int variable_getter (void * context) {
	resource_t* resource = (resource_t*)context;
	return resource->value;
}


int void_setter (void * context, int value) {return 1;}


resource_t* locate_resource (char* name) {
	resource_t* i;

	for (i = resource_head; i; i = i->next) {
		if (!strcmp(i->name, name)) {
			return i;
		}
	}
	return i;
}

