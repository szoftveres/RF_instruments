#ifndef _KEYWORD_H_
#define _KEYWORD_H_


#include "parser.h"

#define MAX_ARGS (4)

typedef enum {
	ARG_NONE,
	ARG_INT,
	ARG_STR
} arg_type_t;


typedef struct keyword_s {
	struct keyword_s *next;
	char* token;
	char* helpstr;
	arg_type_t args[MAX_ARGS];
	int (*exec) (struct keyword_s*, parser_t*);
} keyword_t;


#endif /* INC_KEYWORD_H_ */
