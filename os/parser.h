#ifndef __PARSER__H_
#define __PARSER__H_


#include "lex/lex.h"  // This is somewhat annoying
#include "fifo.h"  // fifo

typedef struct {
	int				line_length;
	char* 			cmdbuf;
	int 			cmd_op; // lex read pointer
	lex_instance_t 	*lex;

} parser_t;


parser_t* parser_create (int line_length);
void parser_destroy (parser_t *parser);

int cmd_line_parser (parser_t *parser, char* line, fifo_t* in, fifo_t* out);
int expression_line_parser (parser_t *parser, char* line, int* n);



#endif
