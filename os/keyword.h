#ifndef _KEYWORD_H_
#define _KEYWORD_H_


#include "fifo.h"



typedef enum cmd_arg_type_e {
	CMD_ARG_TYPE_NONE,

	OBJ_TYPE_NUM,
	OBJ_TYPE_STR,
} cmd_arg_type_t;


typedef struct data_obj_s {
	cmd_arg_type_t type;
	union {
		int 		n;
		char* 		str;
	};
	struct data_obj_s *next;
} data_obj_t;

data_obj_t* obj_add_str (data_obj_t **head, char* str);
data_obj_t* obj_add_num (data_obj_t **head, int n);
data_obj_t * obj_consume (data_obj_t **head);
cmd_arg_type_t get_data_obj_type (data_obj_t *head);


typedef struct cmd_context_t {
	data_obj_t *params;
	data_obj_t *ret;
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
