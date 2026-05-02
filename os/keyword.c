#include "keyword.h"
#include <stddef.h>
#include "hal_plat.h" // t_malloc
#include <string.h> //memcpy


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


keyword_t* function_add (char* token, char* helpstr, int (*exec) (cmd_context_s *ctxt), cmd_arg_type_t ret_type) {

	keyword_t* instance = (keyword_t*)t_malloc(sizeof(keyword_t));
	if (!instance) {
		return instance;
	}
	instance->token = token;  // here we assume that this string is constantly present somewhere
	instance->helpstr = helpstr;  // here we assume that this string is constantly present somewhere
	instance->exec = exec;
	instance->ret_type = ret_type;

	instance->next = keyword_head;
	keyword_head = instance;

	return instance;
}


keyword_t* keyword_add (char* token, char* helpstr, int (*exec) (cmd_context_s *ctxt)) {
    return function_add(token, helpstr, exec, CMD_ARG_TYPE_NONE);
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
		return;
	}
	obj->type = OBJ_TYPE_STR;
	obj->str = t_strdup(str);
    if (head) {
	    obj_insert_end(head, obj);
    }
	return obj;
}


data_obj_t* obj_add_num (data_obj_t **head, int n) {
	data_obj_t *obj = (data_obj_t*)t_malloc(sizeof(data_obj_t));
	if (!obj) {
		return;
	}
	obj->type = OBJ_TYPE_NUM;
	obj->n = n;
    if (head) {
	    obj_insert_end(head, obj);
    }
	return obj;
}


cmd_arg_type_t get_cmd_arg_type (data_obj_t *head) {
	if (!head) {
		return CMD_ARG_TYPE_NONE;
	}
	return head->type;
}


data_obj_t * obj_consume (data_obj_t **head) {
	data_obj_t *current;
	if (!(*head)) {
		return *head;
	}
	current = *head;
	*head = current->next;

	switch (current->type) {
	case OBJ_TYPE_STR:
		t_free(current->str);
		break;
	case OBJ_TYPE_NUM:
		break;
	default:
		break;
	}
	t_free(current);
	return current;
}
