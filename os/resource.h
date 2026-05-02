#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include "globals.h"
#include "keyword.h"


#define MAX_LEN_RESOURCE_NAME (8)

typedef struct resource_s {
	struct resource_s *next;
    data_obj_t *obj;
	char name[MAX_LEN_RESOURCE_NAME];
} resource_t;


resource_t* resource_add (char* name, data_obj_t *obj);
void resource_remove_all (void);

resource_t* locate_resource (char* name);

resource_t* resource_it_start (void);
resource_t* resource_it_next (resource_t* r);

#endif
