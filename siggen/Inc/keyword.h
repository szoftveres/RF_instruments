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


cmd_param_t * cmd_param_consume (cmd_param_t **head);
cmd_arg_type_t get_cmd_arg_type (cmd_param_t **head);
void cmd_param_insert_end (cmd_param_t **head, cmd_param_t *current);



typedef struct keyword_s {
	struct keyword_s *next;
	char* token;
	char* helpstr;
	int (*exec) (cmd_param_t**, fifo_t*, fifo_t*);
} keyword_t;


keyword_t* keyword_it_start (void);
keyword_t* keyword_it_next (keyword_t* kw);


keyword_t* keyword_add (char* token, char* helpstr, int (*exec) (cmd_param_t**, fifo_t*, fifo_t*));
keyword_t* keyword_remove (char* token);
keyword_t* locate_keyword (char* token);

#endif /* INC_KEYWORD_H_ */
