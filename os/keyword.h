#ifndef _KEYWORD_H_
#define _KEYWORD_H_


#include "fifo.h"



typedef enum cmd_arg_type_e {
	CMD_ARG_TYPE_NONE,

	CMD_ARG_TYPE_NUM,
	CMD_ARG_TYPE_STR,
} cmd_arg_type_t;


typedef struct cmd_param_s {
	cmd_arg_type_t type;
	union {
		int 		n;
		char* 		str;
	};
	struct cmd_param_s *next;
} cmd_param_t;

void param_add_str (cmd_param_t **head, char* str);
void param_add_num (cmd_param_t **head, int n);
cmd_param_t * cmd_param_consume (cmd_param_t **head);
cmd_arg_type_t get_cmd_arg_type (cmd_param_t *head);


typedef struct cmd_context_t {
	cmd_param_t *params;
	cmd_param_t *ret;
	fifo_t* in;
	fifo_t* out;
} cmd_context_s;


typedef struct keyword_s {
	struct keyword_s *next;
	char* token;
	char* helpstr;
    cmd_arg_type_t ret_type;
	int (*exec) (cmd_context_s *ctxt);
} keyword_t;


keyword_t* keyword_it_start (void);
keyword_t* keyword_it_next (keyword_t* kw);


keyword_t* function_add (char* token, char* helpstr, int (*exec) (cmd_context_s *ctxt), cmd_arg_type_t ret_type);
keyword_t* keyword_add (char* token, char* helpstr, int (*exec) (cmd_context_s *ctxt));
keyword_t* keyword_remove (char* token);
keyword_t* locate_keyword (char* token);


#endif /* INC_KEYWORD_H_ */
