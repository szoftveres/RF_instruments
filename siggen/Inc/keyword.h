#ifndef _KEYWORD_H_
#define _KEYWORD_H_


#include "parser.h"


typedef struct keyword_s {
	struct keyword_s *next;
	char* token;
	char* helpstr;
	int (*exec) (parser_t*);
} keyword_t;


keyword_t* keyword_it_start (void);
keyword_t* keyword_it_next (keyword_t* kw);


keyword_t* keyword_add (char* token, char* helpstr, int (*exec) (parser_t*));
keyword_t* locate_keyword (char* token);

#endif /* INC_KEYWORD_H_ */
