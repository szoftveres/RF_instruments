#ifndef _KEYWORD_H_
#define _KEYWORD_H_


#include "fifo.h"



typedef enum obj_type_e {
	OBJ_TYPE_NONE,

	OBJ_TYPE_NUM,
	OBJ_TYPE_STR,
} obj_type_t;


typedef struct data_obj_s {
	obj_type_t type;
	union {
		int 		n;
		char* 		str;
	};
	struct data_obj_s *next;
} data_obj_t;

void obj_insert_end (data_obj_t **head, data_obj_t *current);
data_obj_t* obj_add_str (data_obj_t **head, char* str);
data_obj_t* obj_add_num (data_obj_t **head, int n);
data_obj_t* obj_clone (data_obj_t *orig);
void obj_destroy (data_obj_t *obj);
data_obj_t * obj_consume (data_obj_t **head);
obj_type_t get_data_obj_type (data_obj_t *head);


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
	int (*exec) (cmd_context_s *ctxt);
} keyword_t;


keyword_t* keyword_it_start (void);
keyword_t* keyword_it_next (keyword_t* kw);


keyword_t* keyword_add (char* token, char* helpstr, int (*exec) (cmd_context_s *ctxt));
keyword_t* keyword_remove (char* token);
keyword_t* locate_keyword (char* token);


#endif /* INC_KEYWORD_H_ */
