#include "parser.h"
#include "functions.h"
#include <string.h> // strcmp
#include <stdlib.h> //malloc
#include "instances.h"
#include "resource.h"
#include "keyword.h"


static const char* syntax_error = "Syntax error \'%s\'";
static const char* not_an_expression = "Not an expression";



int parser_primary_expression (lex_instance_t *lex, int *n);


int parser_string (lex_instance_t *lex) {
	if (lex->token != T_STRING) {
		return 0;
	}
	str_value(lex); // CMD in lexeme
    return 1;
}


int parser_expect_expression (lex_instance_t *lex, int *n) {
	if (!parser_expression(lex, n) ) {
		console_printf(syntax_error, not_an_expression);
		return 0;
	}
	return 1;
}


int parser_interactive_input_expression (lex_instance_t *lex, int linelen, int *n) {
	int rc;

	if (!lex_get(lex, T_DOLLAR, NULL)) {
		return 0;
	}

	if (!parser_string(lex) ) {
		console_printf("\"prompt\" expected");
		return 0;
	}
	console_printf_e("%s", lex->lexeme);
	next_token(lex);

	char* line = terminal_get_line(online_input, " > ", 1);
	parser_t *lcl_parser = parser_create(linelen);
	if (!lcl_parser) {
		console_printf("malloc fail");
		return 0;
	}

	rc = expression_line_parser(lcl_parser, line, n);

	parser_destroy(lcl_parser);

	return rc;
}


int do_operations (int op_type, int *left, int right) {
    switch (op_type) {
      case T_MUL :
      case T_RECURMUL :
        *left *= right;
        break;
      case T_DIV :
      case T_RECURDIV :
    	*left /= right;
        break;
      case T_MOD :
    	*left %= right;
        break;
      case T_PLUS :
      case T_RECURADD :
    	*left += right;
        break;
      case T_MINUS :
      case T_RECURSUB :
    	*left -= right;
        break;
      case T_SLEFT :
    	*left = *left << right;
        break;
      case T_SRIGHT :
    	*left = *left >> right;
        break;
      case T_EQ :
    	*left = (*left == right);
        break;
      case T_NEQ :
    	*left = (*left != right);
        break;
      case T_GREATER :
        *left = (*left > right);
        break;
      case T_LESS :
    	*left = (*left < right);
        break;
      case T_GREQ :
    	*left = (*left >= right);
        break;
      case T_LEQ :
        *left = (*left <= right);
        break;
      case T_BWAND :
      case T_RECURBWAND :
    	*left &= right;
        break;
      case T_BWXOR :
      case T_RECURBWXOR :
    	*left ^= right;
        break;
      case T_BWOR :
      case T_RECURBWOR :
    	*left |= right;
        break;
      default:
        return 0;
    }
    return 1;
}


int recursive_assignment (lex_instance_t *lex, int left, int *right) {
    int op_type = lex->token;

    switch (op_type) {
      case T_RECURADD :
      case T_RECURSUB :
      case T_RECURMUL :
      case T_RECURDIV :
      case T_RECURBWAND :
      case T_RECURBWOR :
      case T_RECURBWXOR :
        next_token(lex);
        break;
      default:
        return 0;
    }

    if (!parser_expect_expression(lex, right)) {    /* only one expression after assignment */
    	return 0;
    }
    if (!do_operations(op_type, &left, *right)) {
    	return 0;
    }
    *right = left;
    return 1;
}


int get_op_precedence (int op) {
    switch (op) {
      case T_LOR :              /* || */
        return 0;
      case T_LAND :             /* && */
        return 1;
      case T_BWOR :             /* | */
        return 2;
      case T_BWXOR :            /* ^ */
        return 3;
      case T_BWAND :            /* & */
        return 4;
      case T_EQ :               /* == */
      case T_NEQ :              /* != */
        return 5;
      case T_LESS :             /* < */
      case T_GREATER :          /* > */
      case T_LEQ :              /* <= */
      case T_GREQ :             /* >= */
        return 6;
      case T_SLEFT :            /* << */
      case T_SRIGHT :           /* >> */
        return 7;
      case T_PLUS :             /* + */
      case T_MINUS :            /* - */
        return 8;
      case T_MUL :              /* * */
      case T_DIV :              /* / */
      case T_MOD :              /* % */
        return 9;
      default:
        return -1;              /* Not a binary operator */
    }
}


int parser_binary_operation (lex_instance_t *lex, int min_prec, int *left) {
    int op;
    int prec;
    int right;

    prec = get_op_precedence(lex->token);
    if (prec < min_prec) {
        return 0;
    }

    op = lex->token;
    next_token(lex);

    if (!parser_primary_expression(lex, &right)) {
    	console_printf("expected expression after operator");
        return 0;
    }

    /* Subsequent higher precedence operations */
    parser_binary_operation(lex, prec + 1, &right);

    if (!do_operations(op, left, right)) {
    	console_printf("unknown operator");
    	return 0;
    }

    /* Subsequent same or lower precedence operations */
    parser_binary_operation(lex, 0, left);
    return 1;
}


int parser_assignment (lex_instance_t *lex, resource_t *resource) {
	int n;

    if (lex_get(lex, T_ASSIGN, NULL)) {
        if (!parser_expect_expression(lex, &n)) {
        	return 0;
        }
        return resource->set(resource->context, n);;
    } else if (recursive_assignment(lex, resource->get(resource->context), &n)) {
        return resource->set(resource->context, n);;
    }
    return 0;
}


int parser_resource_expression (lex_instance_t *lex, int *n) {
	//int rc;
	if (lex->token != T_IDENTIFIER) {
		return 0;
	}
	resource_t* resource = locate_resource(lex->lexeme);
	if (!resource) {
		return 0;
	}
	next_token(lex);

	//rc = parser_assignment(lex, resource);
	parser_assignment(lex, resource);

	*n = resource->get(resource->context);
	return 1;
}


int parser_numeric_const (lex_instance_t *lex, int *n) {
	if (lex->token != T_CHAR &&
		lex->token != T_HEXA &&
		lex->token != T_INTEGER &&
		lex->token != T_BINARY &&
		lex->token != T_OCTAL) {
		return 0;
	}
	*n = integer_value(lex);
	next_token(lex);
	return 1;
}


int parser_parentheses (lex_instance_t *lex, int *n) {
    if (!lex_get(lex, T_LEFT_PARENTH, NULL)) {
        return 0;
    }
    if (!parser_expect_expression(lex, n)) {
    	return 0;
    }
    if (!lex_get(lex, T_RIGHT_PARENTH, NULL)) {
    	console_printf("expected ')'");
    	return 0;
    }
    return 1;
}


int parser_expression_prefix (lex_instance_t *lex, int *n) {
	int rc = 0;

	if (lex->token == T_MINUS) {
		next_token(lex);
		rc = parser_primary_expression(lex, n);
		*n *= -1;
	} else if (lex->token == T_NEG) {
		next_token(lex);
		rc = parser_primary_expression(lex, n);
		*n = !*n;
	}

	return rc;
}

int parser_primary_expression (lex_instance_t *lex, int *n) {
	int rc = 0;

	if (parser_expression_prefix(lex, n)) {
		rc = 1;
	} else if (parser_parentheses(lex, n)) {
        rc = 1;
    } else if (parser_numeric_const(lex, n)) {
        rc = 1;
    } else if (parser_resource_expression(lex, n)) {
        rc = 1;
    } else if (parser_interactive_input_expression(lex, 40, n)) { // TODO linelen
    	rc = 1;
    }

    return rc;
}


int parser_expression (lex_instance_t *lex, int *n) {
    if (parser_primary_expression(lex, n)) {
    	parser_binary_operation(lex, 0, n);
        return 1;
    }
    return 0;
}


//==============================================================


// this is how the (portable) lex reads a new byte in this type of parser
int parser_lex_read (lex_instance_t *instance, int *b) {
	char byte;
	parser_t *parser = (parser_t*)instance->context; // context of lex is the parser
	if (parser->cmd_op >= parser->line_length) {
		return 0;
	}
	byte = parser->cmdbuf[parser->cmd_op];
	*b = (int)byte;

	if (byte == '\0') {
		return 0;
	}
	parser->cmd_op += 1;
	return 1;
}

// this is how lex prints an error message on this system
void parser_lex_error (lex_instance_t *instance, int c, const char *str) {
	parser_t *parser = (parser_t*)instance->context; // context of lex is the parser
	console_printf("lex error: %s, \"%s\", %i, [%i]", str, parser->lex->lexeme, parser->lex->token, c);
}


// Constructor
parser_t* parser_create (int line_length) {
	parser_t* instance;

	instance = (parser_t*)malloc(sizeof(parser_t));
	if (!instance) {
	    return (instance);
	}
	instance->line_length = line_length;
	instance->cmdbuf = (char*)malloc(instance->line_length);
	if (!instance->cmdbuf) {
		free(instance);
		instance = NULL;
		return instance;
	}
	instance->cmd_op = 0;
	instance->lex = lex_create(instance, instance->line_length, parser_lex_read, parser_lex_error, 0x00); // context of lex is the parser
	if (!instance->lex) {
		free(instance->cmdbuf);
		free(instance);
	    return NULL;
	}
	return instance;
}

// Destructor
void parser_destroy (parser_t *parser) {
	if (!parser) {
		return;
	}
	lex_destroy(parser->lex);
	free(parser->cmdbuf);
	free(parser);
}


//==============================================================


int parser_keyword_statement (parser_t *parser) {
	int rc = 0;

	if (parser->lex->token != T_IDENTIFIER) {
		return 0;
	}
	keyword_t* keyword = locate_keyword(parser->lex->lexeme);
	if (!keyword) {
		console_printf("Bad cmd \'%s\'", parser->lex->lexeme);
		next_token(parser->lex);
		return 0;
	}
	next_token(parser->lex);
	if (!keyword->exec) {
		return rc;
	}
	rc = keyword->exec(parser);
	if (rc < 1) {
		console_printf("%s %s", keyword->token, keyword->helpstr);
	}
	return rc;
}


int parser_programline_statement (parser_t *parser) {
	if (parser->lex->token != T_INTEGER) { // edit a program line
		return 0;
	}
	int nline = integer_value(parser->lex);
	if (nline < 0 || nline >= program->header.fields.nlines) {
		console_printf("Bad line \'%i\'", nline);
		return 0;
	}
	next_token(parser->lex);
	char* line = program_line(program, nline);

	if (!parser_string(parser->lex)) {
		console_printf("\"cmd\" expected");
		return 0;
	}
	if ((strlen(parser->lex->lexeme) + 1) < program->header.fields.linelen) {
		strcpy(line, parser->lex->lexeme);
	} else {
		console_printf("too long");
	}
	next_token(parser->lex);
	//cmd_program_list(parser);

	return 1;
}


int parser_statement (parser_t *parser) {
	int rc = 0;
	int n;

	 if (parser->lex->token == T_SEMICOLON) {  // Empty statement
		rc = 1;
	} else if (parser_programline_statement(parser)) {
		rc = 1;
	} else if (parser_resource_expression(parser->lex, &n)) {
		//console_printf("%i", n);
		rc = 1;
	} else if (parser_keyword_statement(parser)) {
		rc = 1;
	}

	return rc;
}


int cmd_line_parser (parser_t *parser, char* line) {
	int rc = 1;

	if (!line) {
		return 0;
	}
	strcpy(parser->cmdbuf, line);

	parser->cmd_op = 0;
	lex_reset(parser->lex);

	do {
		if (lex_get(parser->lex, T_EOF, NULL)) {
			return 1; // Empty line is valid
		}
		rc = parser_statement(parser);
	} while (rc && lex_get(parser->lex, T_SEMICOLON, NULL));

	return rc;
}


int expression_line_parser (parser_t *parser, char* line, int* n) {
	int rc = 1;

	if (!line) {
		return 0;
	}
	strcpy(parser->cmdbuf, line);

	parser->cmd_op = 0;
	lex_reset(parser->lex);

	do {
		rc = parser_expect_expression(parser->lex, n);
	} while (rc && lex_get(parser->lex, T_COMMA, NULL));

	return rc;
}

