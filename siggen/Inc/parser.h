#ifndef __PARSER__H_
#define __PARSER__H_


#include "../lex/lex.h"  // This is somewhat annoying

typedef struct {
	int				line_length;
	char* 			cmdbuf;
	int 			cmd_op; // lex read pointer
	lex_instance_t 	*lex;
} parser_t;


int parser_expect_expression (lex_instance_t *lex, int *n);
int parser_expression (lex_instance_t *lex, int *n);
int parser_string (lex_instance_t *lex);


parser_t* parser_create (int line_length);
void parser_destroy (parser_t *parser);

int cmd_line_parser (parser_t *parser, char* line);
int expression_line_parser (parser_t *parser, char* line, int* n);



#endif
