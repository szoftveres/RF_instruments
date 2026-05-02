#include "keyword.h"
#include <stddef.h>
#include "hal_plat.h" // t_malloc
#include <string.h> //memcpy
#include <stdlib.h> // malloc free

keyword_t *keyword_head = NULL;


keyword_t* keyword_it_start (void) {
	return keyword_head;
}


keyword_t* keyword_it_next (keyword_t* kw) {
	if (kw) {
		kw = kw->next;
	}
	return kw;
}


keyword_t* keyword_add (char* token, char* helpstr, int (*exec) (cmd_context_s *ctxt)) {

	keyword_t* instance = (keyword_t*)t_malloc(sizeof(keyword_t));
	if (!instance) {
		return instance;
	}
	instance->token = token;  // here we assume that this string is constantly present somewhere
	instance->helpstr = helpstr;  // here we assume that this string is constantly present somewhere
	instance->exec = exec;

	instance->next = keyword_head;
	keyword_head = instance;

	return instance;
}


keyword_t* keyword_remove (char* token) {
	keyword_t **p;
	keyword_t *r;

	for (p = &keyword_head; *p; p = &((*p)->next)) {
		if (!strcmp((*p)->token, token)) {
			r = *p;
			*p = (*p)->next;
			return r;
		}
	}
	return NULL;
}


keyword_t* locate_keyword (char* token) {
	keyword_t *i;

	for (i = keyword_head; i; i = i->next) {
		if (!strcmp(i->token, token)) {
			return i;
		}
	}
	return i;
}

// ==================================================


void obj_insert_end (data_obj_t **head, data_obj_t *current) {
    while (*head) {head = &((*head)->next);}
    current->next = *head;
    *head = current;
}


data_obj_t* obj_add_str (data_obj_t **head, char* str) {
	data_obj_t *obj = (data_obj_t*)t_malloc(sizeof(data_obj_t));
	if (!obj) {
		return obj;
	}
	obj->type = OBJ_TYPE_STR;
	obj->str = strdup(str); // XXX t_strdup
    if (head) {
	    obj_insert_end(head, obj);
    }
	return obj;
}


data_obj_t* obj_add_num (data_obj_t **head, int n) {
	data_obj_t *obj = (data_obj_t*)t_malloc(sizeof(data_obj_t));
	if (!obj) {
		return obj;
	}
	obj->type = OBJ_TYPE_NUM;
	obj->n = n;
    if (head) {
	    obj_insert_end(head, obj);
    }
	return obj;
}


data_obj_t* obj_clone (data_obj_t *orig) {
    data_obj_t *obj = (data_obj_t*)t_malloc(sizeof(data_obj_t));
    if (!obj) {
        return obj;
    }
    memcpy(obj, orig, sizeof(data_obj_t));
    obj->next = NULL;
    switch (orig->type) {
      case OBJ_TYPE_STR:
        obj->str = strdup(orig->str);  // XXX t_free
        break;
    }
    return obj;
}


obj_type_t get_data_obj_type (data_obj_t *head) {
	if (!head) {
		return OBJ_TYPE_NONE;
	}
	return head->type;
}


void obj_destroy (data_obj_t *obj) {
    switch (obj->type) {
      case OBJ_TYPE_STR:
        free(obj->str);  // XXX t_free
        break;
    }
    t_free(obj);
}

data_obj_t * obj_consume (data_obj_t **head) {
	data_obj_t *current;
	if (!(*head)) {
		return *head;
	}
	current = *head;
	*head = current->next;

    obj_destroy(current);
	return current;
}
