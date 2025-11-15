#include "keyword.h"
#include <stddef.h>
#include <stdlib.h> //malloc
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


keyword_t* keyword_add (char* token, char* helpstr, int (*exec) (parser_t*)) {

	keyword_t* instance = (keyword_t*)malloc(sizeof(keyword_t));
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


keyword_t* locate_keyword (char* token) {
	keyword_t* i;

	for (i = keyword_head; i; i = i->next) {
		if (!strcmp(i->token, token)) {
			return i;
		}
	}
	return i;
}
