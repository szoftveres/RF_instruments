#ifndef __PARSER__H_
#define __PARSER__H_


#include "../lex/lex.h"  // This is somewhat annoying


typedef struct {
	int				line_length;
	char* 			cmdbuf;
	int 			cmd_ip; // write pointer
	int 			cmd_op; // lex read pointer
	lex_instance_t 	*lex;
} parser_t;



parser_t* parser_create (int line_length);
void parser_destroy (parser_t *parser);

int parser_fill (parser_t *parser, char b);
void parser_back (parser_t *parser);
int parser_run (parser_t *parser);



#endif
