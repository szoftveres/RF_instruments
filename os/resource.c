#include "resource.h"
#include "hal_plat.h" // t_malloc
#include <string.h> //strcmp


static resource_t *resource_head = NULL;
static int resource_counter = 0;


resource_t* resource_add (char* name, data_obj_t *obj) {

	resource_t* instance = (resource_t*)t_malloc(sizeof(resource_t));
	if (!instance) {
		return instance;
	}
	strncpy(instance->name, name, MAX_LEN_RESOURCE_NAME);

    resource_counter += (obj ? 2 : 1); // simple t_malloc counter

    instance->obj = obj;

	instance->next = resource_head;
	resource_head = instance;

	return instance;
}


void resource_remove_all (void) {
    while (resource_head) {
        resource_t* instance = resource_head;
        resource_head = instance->next;
        resource_counter -= (instance->obj ? 2 : 1); // simple t_malloc counter
        obj_destroy(instance->obj);
        t_free(instance);
    }
}


int rsrc_count (void) {
    return resource_counter;
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

