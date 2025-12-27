#include "keyword.h"
#include <stddef.h>
#include "hal_plat.h" //malloc
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


keyword_t* keyword_add (char* token, char* helpstr, int (*exec) (cmd_param_t**, fifo_t*, fifo_t*)) {

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



void cmd_param_insert_end (cmd_param_t **head, cmd_param_t *current) {
	while (*head) {head = &((*head)->next);}
	current->next = *head;
	*head = current;
}

cmd_arg_type_t get_cmd_arg_type (cmd_param_t **head) {
	if (!(*head)) {
		return CMD_ARG_TYPE_NONE;
	}
	return (*head)->type;
}


cmd_param_t * cmd_param_consume (cmd_param_t **head) {
	cmd_param_t *current;
	if (!(*head)) {
		return *head;
	}
	current = *head;
	*head = current->next;

	switch (current->type) {
	case CMD_ARG_TYPE_STR:
		//console_printf("str:%s", current->str);
		t_free(current->str);
		break;
	case CMD_ARG_TYPE_NUM:
		//console_printf("num:%i", current->n);
		break;
	default:
		break;
	}
	t_free(current);
	return current;
}
