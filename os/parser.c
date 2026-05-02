#include "parser.h"
#include "hal_plat.h" // t_malloc
#include <string.h> // strcmp
#include "globals.h"
#include "resource.h"
#include "keyword.h"



static const char* syntax_error = "Syntax error \'%s\'\n";
static const char* not_an_expression = "Numeric expression expected\n";
static const char* illegal_operator = "Illegal operator\n";


data_obj_t* parser_primary_expression (lex_instance_t *lex);
data_obj_t* parser_expression (lex_instance_t *lex);


data_obj_t* parser_expect_expression (lex_instance_t *lex) {
    data_obj_t *obj = NULL;
	if (!(obj = parser_expression(lex))) {
		printf_f(STDERR, syntax_error, not_an_expression);
	}
	return obj;
}


void parser_build_param_list (parser_t *parser, data_obj_t **head) {
    int rc;
    data_obj_t *obj = NULL;

    do {
        rc = 0;
        if ((obj = parser_expression(parser->lex))) {
            obj_insert_end(head, obj);
            rc = 1;
        }
    } while (rc);

    return;
}



data_obj_t* parser_function (lex_instance_t *lex) {
    data_obj_t* obj = NULL;
	if (lex->token != T_IDENTIFIER) {
		return NULL;
	}
	parser_t *parser = (parser_t *)(lex->context);

    keyword_t *kw;
	for (kw = keyword_it_start(); kw; kw = keyword_it_next(kw)) {
		if (!strcmp(lex->lexeme, kw->token)) {
			int rc;

			next_token(lex);

		    if (!lex_get(lex, T_LEFT_PARENTH, NULL)) {
                printf_f(STDERR, "expected '('\n");
		        return obj;
		    }

			cmd_context_s *cmd_ctxt = (cmd_context_s*)t_malloc(sizeof(cmd_context_s));
			cmd_ctxt->params = NULL;
			parser_build_param_list(parser, &(cmd_ctxt->params));

		    if (!lex_get(lex, T_RIGHT_PARENTH, NULL)) {
                printf_f(STDERR, "expected ')'\n");
                t_free(cmd_ctxt);
                return obj;
		    }

		    cmd_ctxt->ret = NULL;
		    rc = kw->exec(cmd_ctxt);

			while (obj_consume(&(cmd_ctxt->params))) {
				printf_f(STDERR, "%s: unused parameter\n", kw->token);
			}

            if (rc) {
                obj = cmd_ctxt->ret;
                cmd_ctxt->ret = NULL;
            }

            while (obj_consume(&(cmd_ctxt->ret)));

			t_free(cmd_ctxt);

		    return obj;
		}
	}
	return 0;
}


data_obj_t* op_str_str (int op_type, data_obj_t *left, data_obj_t *right) {
    int n;
    switch (op_type) {
      case T_PLUS :
      case T_RECURADD :
    	char* buf = (char*)t_malloc(strlen(left->str) + strlen(right->str) + 1);
    	strcpy(buf, left->str);
    	obj_destroy(left);
        strcat(buf, right->str);
        left = obj_add_str(NULL, buf);
        t_free(buf);
        break;
      case T_EQ :
        n = strcmp(left->str, right->str) ? 0 : 1;
        obj_destroy(left);
        left = obj_add_num(NULL, n);
        break;
      case T_NEQ :
        n = strcmp(left->str, right->str) ? 1 : 0;
        obj_destroy(left);
        left = obj_add_num(NULL, n);
        break;
      default:
        obj_destroy(left);
        left = NULL;
        printf_f(STDERR, illegal_operator);
        break;
    }
    obj_destroy(right);
    return left;
}


data_obj_t* op_num_num (int op_type, data_obj_t *left, data_obj_t *right) {
    switch (op_type) {
      case T_MUL :
      case T_RECURMUL :
        left->n *= right->n;
        break;
      case T_DIV :
      case T_RECURDIV :
        left->n /= right->n;
        break;
      case T_MOD :
        left->n %= right->n;
        break;
      case T_PLUS :
      case T_RECURADD :
        left->n += right->n;
        break;
      case T_MINUS :
      case T_RECURSUB :
        left->n -= right->n;
        break;
      case T_SLEFT :
        left->n = left->n << right->n;
        break;
      case T_SRIGHT :
        left->n = left->n >> right->n;
        break;
      case T_EQ :
        left->n = (left->n == right->n);
        break;
      case T_NEQ :
        left->n = (left->n != right->n);
        break;
      case T_GREATER :
        left->n = (left->n > right->n);
        break;
      case T_LESS :
        left->n = (left->n < right->n);
        break;
      case T_GREQ :
        left->n = (left->n >= right->n);
        break;
      case T_LEQ :
        left->n = (left->n <= right->n);
        break;
      case T_BWAND :
      case T_RECURBWAND :
        left->n &= right->n;
        break;
      case T_BWXOR :
      case T_RECURBWXOR :
        left->n ^= right->n;
        break;
      case T_BWOR :
      case T_RECURBWOR :
        left->n |= right->n;
        break;
      default:
        obj_destroy(left);
        left = NULL;
        printf_f(STDERR, illegal_operator);
        break;
    }
    obj_destroy(right);
    return left;
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


      case T_RECURADD :
      case T_RECURSUB :
        return 10;
      case T_RECURMUL :
      case T_RECURDIV :
        return 11;
      case T_RECURBWAND :
      case T_RECURBWOR :
      case T_RECURBWXOR :
        return 12;

      default:
        return -1;              /* Not a binary operator */
    }
}


data_obj_t* parser_binary_operation (lex_instance_t *lex, int min_prec, data_obj_t *left) {
    int op;
    int prec;
    data_obj_t *right = NULL;

    prec = get_op_precedence(lex->token);
    if (prec < min_prec) {
        return left;
    }

    op = lex->token;
    next_token(lex);

    if (!(right = parser_primary_expression(lex))) {
        printf_f(STDERR, "expected expression after operator\n");
        obj_destroy(left);
        return NULL;
    }

    /* Subsequent higher precedence operations */
    right = parser_binary_operation(lex, prec + 1, right);
    if (!right) {
        obj_destroy(left);
        return NULL;
    }

    if ((left->type == OBJ_TYPE_NUM) && (right->type == OBJ_TYPE_NUM)) {
        left = op_num_num(op, left, right);
    } else if ((left->type == OBJ_TYPE_STR) && (right->type == OBJ_TYPE_STR)) {
        left = op_str_str(op, left, right);
    } else {
        printf_f(STDERR, illegal_operator);
        obj_destroy(right);
        obj_destroy(left);
        return NULL;
    }

    if (!left) {
        return NULL;
    }

    /* Subsequent same or lower precedence operations */
    left = parser_binary_operation(lex, 0, left);
    return left;
}


int recursive_assignment (lex_instance_t *lex) {
    int op_type = lex->token;

    switch (op_type) {
      case T_RECURADD :
      case T_RECURSUB :
      case T_RECURMUL :
      case T_RECURDIV :
      case T_RECURBWAND :
      case T_RECURBWOR :
      case T_RECURBWXOR :
        //next_token(lex);
        return 1;
    }
    return 0;
}


data_obj_t* parser_assignment (lex_instance_t *lex, data_obj_t* left) {
    data_obj_t* right = NULL;

    if (lex_get(lex, T_ASSIGN, NULL)) {
        right = parser_expect_expression(lex);
        if (left) {
            obj_destroy(left);
        }
    } else if (left && recursive_assignment(lex)) {
        data_obj_t *saved_original = obj_clone(left);
        if (!(right = parser_binary_operation(lex, 0, left))) {
            right = saved_original;
        } else {
            obj_destroy(saved_original);
        }
    }
    return right;
}


data_obj_t* parser_resource_expression (lex_instance_t *lex) {
    data_obj_t* obj;
	if (lex->token != T_IDENTIFIER) {
		return NULL;
	}
    char* name = t_strdup(lex->lexeme);
	next_token(lex);

	resource_t* resource = locate_resource(name);
	if (!resource) { // Rsrc doens't exist, create it by assignment
        data_obj_t* obj;
        if (!(obj = parser_assignment(lex, NULL))) {
            t_free(name);
		    return NULL;
        }
        resource = resource_add(name, obj);
	} else { // Rsrc exists
        if ((obj = parser_assignment(lex, resource->obj))) {
            resource->obj = obj;
        }
    }
    t_free(name);

	return obj_clone(resource->obj);
}


data_obj_t* parser_string (lex_instance_t *lex) {
    data_obj_t* obj = NULL;
    if (lex->token == T_STRING) {
        str_value(lex);
        obj = obj_add_str(NULL, lex->lexeme);
        next_token(lex);
    }
    return obj;
}


data_obj_t* parser_numeric_const (lex_instance_t *lex) {
    int n;
	if (lex->token != T_CHAR &&
		lex->token != T_HEXA &&
		lex->token != T_INTEGER &&
		lex->token != T_BINARY &&
		lex->token != T_OCTAL) {
		return 0;
	}
	n = integer_value(lex);
	next_token(lex);
	return obj_add_num(NULL, n);
}


data_obj_t* parser_parentheses (lex_instance_t *lex) {
    data_obj_t* obj = NULL;
    if (!lex_get(lex, T_LEFT_PARENTH, NULL)) {
        return obj;
    }
    obj = parser_expect_expression(lex);
    if (!lex_get(lex, T_RIGHT_PARENTH, NULL)) {
        printf_f(STDERR, "expected ')'\n");
        obj_destroy(obj);
        return NULL;
    }
    return obj;
}


data_obj_t* parser_expression_prefix (lex_instance_t *lex) {
    data_obj_t *obj = NULL;

	if (lex->token == T_MINUS) {
		next_token(lex);
		obj = parser_primary_expression(lex);
        if (!obj) {
            printf_f(STDERR, not_an_expression);
            return obj;
        }
        if (obj->type != OBJ_TYPE_NUM) {
            printf_f(STDERR, not_an_expression);
            obj_destroy(obj);
            return NULL;
        }
        obj->n = -(obj->n);
	} else if (lex->token == T_NEG) {
		next_token(lex);
		obj = parser_primary_expression(lex);
        if (!obj) {
            printf_f(STDERR, not_an_expression);
            return obj;
        }
        if (obj->type != OBJ_TYPE_NUM) {
            printf_f(STDERR, not_an_expression);
            obj_destroy(obj);
            return NULL;
        }
		obj->n = !(obj->n);
	}

	return obj;
}


data_obj_t* parser_primary_expression (lex_instance_t *lex) {
    data_obj_t *obj = NULL;

	       if ((obj = parser_expression_prefix(lex))) {
	} else if ((obj = parser_parentheses(lex))) {
    } else if ((obj = parser_numeric_const(lex))) {
    } else if ((obj = parser_string(lex))) {
    } else if ((obj = parser_function(lex))) {
    } else if ((obj = parser_resource_expression(lex))) {
    }

    return obj;
}


data_obj_t* parser_expression (lex_instance_t *lex) {
    data_obj_t* obj = NULL;
    if ((obj = parser_primary_expression(lex))) {
        obj = parser_binary_operation(lex, 0, obj);
    }
    return obj;
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

	if ((byte == '\0') || (byte == '\n')) {
		return 0;
	}
	parser->cmd_op += 1;
	return 1;
}

// this is how lex prints an error message on this system
void parser_lex_error (lex_instance_t *instance, int c, const char *str) {
	printf_f(STDERR, "lex error: %s, \"%s\", %i, [%i]\n", str, instance->lexeme, instance->token, c);
}


// Constructor
parser_t* parser_create (int line_length) {
	parser_t* instance;

	instance = (parser_t*)t_malloc(sizeof(parser_t));
	if (!instance) {
	    return (instance);
	}
	instance->line_length = line_length;
	instance->cmdbuf = (char*)t_malloc(instance->line_length);
	if (!instance->cmdbuf) {
		t_free(instance);
		instance = NULL;
		return instance;
	}
	instance->cmd_op = 0;
	instance->cmd_ip = 0;
	instance->lex = lex_create(instance, instance->line_length, parser_lex_read, parser_lex_error, 0x00); // context of lex is the parser
	if (!instance->lex) {
		t_free(instance->cmdbuf);
		t_free(instance);
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
	t_free(parser->cmdbuf);
	t_free(parser);
}


//==============================================================


keyword_t *parser_valid_keyword (parser_t *parser) {
	keyword_t* keyword;

	if (parser->lex->token != T_IDENTIFIER) {
		return NULL;
	}
	keyword = locate_keyword(parser->lex->lexeme);
	if (!keyword) {
		return NULL;
	}
	next_token(parser->lex);
	if (!keyword->exec) {
		printf_f(STDERR, "Invalid cmd\n");
		return NULL;
	}
	return keyword;
}


int parser_keyword_train (parser_t *parser, fifo_t* in, fifo_t* out) {
	int rc;
	int cont;
	keyword_t*  kw;
	fifo_t* pipes = NULL;

	do {
		cont = 0;
		fifo_t* out_new = out;
		kw = parser_valid_keyword(parser);
		if (!kw) {
			rc = 0;
			break;
		}

		cmd_context_s *cmd_ctxt = (cmd_context_s*)t_malloc(sizeof(cmd_context_s));
		cmd_ctxt->params = NULL;
		parser_build_param_list(parser, &(cmd_ctxt->params));

		if (lex_get(parser->lex, T_ARROW, NULL)) {
			out_new = fifo_create(512, sizeof(uint16_t));
			out_new->next = pipes;
			pipes = out_new;
			cont = 1;
		}

		cmd_ctxt->in = in;
		cmd_ctxt->out = out_new;
		cmd_ctxt->ret = NULL;

		rc = kw->exec(cmd_ctxt);
		in = out_new;

		if (!rc) {
			printf_f(STDERR, "%s %s\n", kw->token, kw->helpstr);
		}
		while (obj_consume(&(cmd_ctxt->params))) {
			printf_f(STDERR, "unused parameter\n");
		}
		while (obj_consume(&(cmd_ctxt->ret))) {
			printf_f(STDERR, "unused retval\n");
		}
		t_free(cmd_ctxt);
	} while ((rc > 0) && cont);

	while (scheduler_burst_run(scheduler)) {
		if (switchbreak()) {
			scheduler_killall(scheduler);
		}
	}

	while (pipes) {
		fifo_t* current = pipes;
		pipes = current->next;
		fifo_destroy(current);
	}

	return rc;
}


int parser_programline_statement (parser_t *parser) {
	if (parser->lex->token != T_INTEGER) { // edit a program line
		return 0;
	}
	int nline = integer_value(parser->lex);
	if (nline < 0 || nline >= program->header.fields.nlines) {
		printf_f(STDERR, "Bad line \'%i\'\n", nline);
		return 0;
	}
	next_token(parser->lex);
	char* line = program_line(program, nline);

    if (parser->lex->token != T_STRING) {
		printf_f(STDERR, "\"cmd\" expected\n");
		return 0;
	}
    str_value(parser->lex);

	if ((strlen(parser->lex->lexeme) + 1) < program->header.fields.linelen) {
		strcpy(line, parser->lex->lexeme);
	} else {
		printf_f(STDERR, "too long\n");
	}
    next_token(parser->lex);
	//cmd_program_list(parser);

	return 1;
}


int parser_statement (parser_t *parser, fifo_t* in, fifo_t* out) {
	int rc = 0;
    data_obj_t* obj;

	 if (parser->lex->token == T_SEMICOLON) {  // Empty statement
		rc = 1;
	} else if (parser_programline_statement(parser)) {
		rc = 1;
    } else if (parser_keyword_train(parser, in, out)) {
        rc = 1;
    } else if ((obj = parser_resource_expression(parser->lex))) {
        obj_destroy(obj);
		rc = 1;
	} else {
        printf_f(STDERR, "unrecognized command\n");
    }

	return rc;
}


int cmd_line_parser (parser_t *parser, char* line, fifo_t* in, fifo_t* out) {
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
		rc = parser_statement(parser, in, out);
	} while ((rc > 0) && lex_get(parser->lex, T_SEMICOLON, NULL));

	return rc;
}

