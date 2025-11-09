#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include "instances.h"


#define MAX_LEN_RESOURCE_NAME (8)

typedef struct resource_s {
	struct resource_s *next;
	void *context;
	int value;
	void (*set) (void *, int);
	int (*get) (void *);
	char name[MAX_LEN_RESOURCE_NAME];
} resource_t;


resource_t* resource_add (char* name, void* context, void (*setter) (void *, int), int (*getter) (void *));

resource_t* locate_resource (char* name);

resource_t* resource_it_start (void);
resource_t* resource_it_next (resource_t* r);

void variable_setter (void * context, int value);
int variable_getter (void * context);

void void_setter (void * context, int value);

#endif
