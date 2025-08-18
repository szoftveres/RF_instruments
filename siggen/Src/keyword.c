#include "keyword.h"
#include <stddef.h>
#include <stdlib.h> //malloc
#include <string.h> //memcpy


keyword_t *keyword_head = NULL;

keyword_t* keyword_add (char* token, char* helpstr, arg_type_t* args, int (*exec) (keyword_t*, parser_t*)) {

	keyword_t* instance = (keyword_t*)malloc(sizeof(keyword_t));
	if (!instance) {
		return instance;
	}
	instance->token = token;  // here we assume that this string is constantly present somewhere
	instance->helpstr = helpstr;  // here we assume that this string is constantly present somewhere
	memcpy(instance->args, args, sizeof(instance->args));

	instance->next = keyword_head;
	keyword_head = instance;

	return instance;
}


keyword_t* locate_keyword (char* token) {
	keyword_t* i;

	for (i = keyword_head; i; i = i->next) {
		if (!strcmp(i->token, token)) {
			return i;
		}
	}
	return i;
}
