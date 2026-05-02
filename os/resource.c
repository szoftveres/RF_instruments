#include "resource.h"
#include "hal_plat.h" // t_malloc
#include <string.h> //strcmp


static resource_t *resource_head = NULL;


resource_t* resource_add (char* name, data_obj_t *obj) {

	resource_t* instance = (resource_t*)t_malloc(sizeof(resource_t));
	if (!instance) {
		return instance;
	}
	strncpy(instance->name, name, MAX_LEN_RESOURCE_NAME);

    instance->obj = obj;

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



resource_t* locate_resource (char* name) {
	resource_t* i;

	for (i = resource_head; i; i = i->next) {
		if (!strcmp(i->name, name)) {
			return i;
		}
	}
	return i;
}

