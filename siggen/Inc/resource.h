#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include "instances.h"


typedef struct resource_s {
	void *context;
	void (*set) (void *, int);
	int (*get) (void *);
} resource_t;


typedef struct variable_s {
	int value;
	resource_t *resource;
} variable_t;


resource_t* resource_create (void* context, void (*setter) (void *, int), int (*getter) (void *));

void resource_destroy (resource_t* instance);

void resources_setup (void);

resource_t* locate_resource (char* identifier);



#endif
