#ifndef __PARSER__H_
#define __PARSER__H_


#include "lex/lex.h"  // This is somewhat annoying
#include "fifo.h"  // fifo
#include "keyword.h"  // obj



typedef struct {
	int				line_length;
	char* 			cmdbuf;
	int 			cmd_ip;
	int 			cmd_op; // lex read pointer
	lex_instance_t 	*lex;
} parser_t;


parser_t* parser_create (int line_length);
void parser_destroy (parser_t *parser);

int cmd_line_parser (parser_t *parser, fifo_t* in, fifo_t* out);
data_obj_t* expr_line_parser (parser_t *parser);



#endif
