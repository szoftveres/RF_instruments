#include <parser.h>
#include "functions.h"
#include <string.h> // strcmp
#include <stdlib.h> //malloc
#include <stdio.h> // EOF
#include "instances.h"
#include "resource.h"
#include "stm32f4xx_hal.h" // HAL_Delay


static const char* invalid_val = "Invalid value \'%i\'";
static const char* not_a_string = "Not a string";
static const char* syntax_error = "Syntax error \'%s\'";
static const char* not_an_expression = "Not an expression";




int parser_expression (parser_t *parser, int *n);
int parser_primary_expression (parser_t *parser, int *n);


int parser_resource_expression (parser_t *parser, int *n) {
	if (parser->lex->token != T_IDENTIFIER) {
		return 0;
	}
	resource_t* resource = locate_resource(parser->lex->lexeme);
	if (!resource) {
		return 0;
	}
	next_token(parser->lex);
	*n = resource->get(resource->context);
	return 1;
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



int recursive_assignment (parser_t *parser, int left, int *right) {
    int op_type = parser->lex->token;

    switch (op_type) {
      case T_RECURADD :
      case T_RECURSUB :
      case T_RECURMUL :
      case T_RECURDIV :
      case T_RECURBWAND :
      case T_RECURBWOR :
      case T_RECURBWXOR :
        next_token(parser->lex);
        break;
      default:
        return 0;
    }

    if (!parser_expression(parser, right)) {    /* only one expression after assignment */
    	console_printf("expected expression after assignment operator");
    }
    if (!do_operations(op_type, &left, *right)) {
    	return 0;
    }
    *right = left;
    return 1;
}



int parser_assignment (parser_t *parser, resource_t *resource) {
	int n;

    if (lex_get(parser->lex, T_ASSIGN, NULL)) {
        if (!parser_expression(parser, &n)) {
        	console_printf("expected expression after '='");
        }
        resource->set(resource->context, n);
        return 1;
    } else if (recursive_assignment(parser, resource->get(resource->context), &n)) {
    	resource->set(resource->context, n);
        return 1;
    }
    return 0;
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


int parser_binary_operation (parser_t *parser, int min_prec, int *left) {
    int op;
    int prec;
    int right;

    prec = get_op_precedence(parser->lex->token);
    if (prec < min_prec) {
        return 0;
    }

    op = parser->lex->token;
    next_token(parser->lex);

    if (!parser_primary_expression(parser, &right)) {
    	console_printf("expected expression after operator");
        return 0;
    }

    /* Subsequent higher precedence operations */
    parser_binary_operation(parser, prec + 1, &right);

    if (!do_operations(op, left, right)) {
    	console_printf("unknown operator");
    	return 0;
    }

    /* Subsequent same or lower precedence operations */
    parser_binary_operation(parser, 0, left);
    return 1;
}


int parser_numeric_const (parser_t *parser, int *n) {
	int neg = 0;
	if (parser->lex->token == T_MINUS) {
		neg = 1;
		next_token(parser->lex);
	}
	if (parser->lex->token != T_CHAR &&
		parser->lex->token != T_HEXA &&
		parser->lex->token != T_INTEGER &&
		parser->lex->token != T_BINARY &&
		parser->lex->token != T_OCTAL) {
		return 0;
	}
	*n = integer_value(parser->lex) * (neg ? -1 : 1);
	next_token(parser->lex);
	return 1;
}


int parser_parentheses (parser_t *parser, int *n) {
    if (!lex_get(parser->lex, T_LEFT_PARENTH, NULL)) {
        return 0;
    }
    if (!parser_expression(parser, n)) {
    	console_printf("expected expression after '('");
    	return 0;
    }
    if (!lex_get(parser->lex, T_RIGHT_PARENTH, NULL)) {
    	console_printf("expected ')'");
    	return 0;
    }
    return 1;
}


int parser_primary_expression (parser_t *parser, int *n) {
    if (parser_parentheses(parser, n)) {
        return 1;
    }
    if (parser_numeric_const(parser, n)) {
        return 1;
    }
    if (parser_resource_expression(parser, n)) {
        return 1;
    }
    return 0;
}


int parser_expression (parser_t *parser, int *n) {
    if (parser_primary_expression(parser, n)) {
    	parser_binary_operation(parser, 0, n);
        return 1;
    }
    return 0;
}


int parser_string (parser_t *parser, char **s) {
	if (parser->lex->token != T_STRING) {
		*s = NULL;
		return 0;
	}
	str_value(parser->lex); // CMD in lexeme
	*s = parser->lex->lexeme;
    return 1;
}



// Recursive "parser within parser"
int parse_str_cmd (parser_t *parser, char* cmdstr) {
	int rc = 1;
	parser_t *lcl_parser = parser_create(parser->line_length);
	if (!lcl_parser) {
		console_printf("malloc fail");
		return 0;
	}
	// copying the command string to the new parser
	char* lp = cmdstr;
	while (*lp) {
		parser_fill(lcl_parser, *lp++);
	}
	parser_fill(lcl_parser, EOF);

	rc = parser_run(lcl_parser);


	parser_destroy(lcl_parser);
	return rc;
}


int parser_if (parser_t *parser) {
	int n;
	char* cmdstr;
	if (!parser_expression(parser, &n) ) {
		console_printf(syntax_error, not_an_expression);
		return 0;
	}
	if (!parser_string(parser, &cmdstr)) {
		console_printf(not_a_string);
		return 0;
	}
	if (!n) {
		return 1; // condition is false
	}
	if (!parse_str_cmd(parser, cmdstr)) {
		return 0;
	}
	next_token(parser->lex);
	return 1;
}


//==============================================================


int cmd_eeprom_read (parser_t *parser) {
	int addr;
	if (!parser_expression(parser, &addr) ) {
		console_printf(syntax_error, not_an_expression);
		return 0;
	}
	console_printf_e("%04x | ", addr);

	eeprom->read_block(eeprom, addr);

	for (int i = 0; i != eeprom->blocksize; i++) {
		console_printf_e("%02x ", eeprom->buffer[i]);
	}
	/*
	console_printf_e("| ");
	lcl_addr = addr;
	for (int i = 0; i != 16; i++) {
		b = eeprom_read_byte((uint16_t)lcl_addr);
		console_printf_e("%c", b);
		lcl_addr++;
	}
	*/
	console_printf(" |");
	return 1;
}


int cmd_show_cfg (parser_t *parser) {
	print_cfg();
	return 1;
}


int cmd_rfon (parser_t *parser) {
	set_rf_output(1);
	if (config.fields.echoon) {
		print_cfg();
	}
	return 1;
}


int cmd_rfoff (parser_t *parser) {
	set_rf_output(0);
	if (config.fields.echoon) {
		print_cfg();
	}
	return 1;
}


int cmd_echoon (parser_t *parser) {
	config.fields.echoon = 1;
	return 1;
}


int cmd_echooff (parser_t *parser) {
	config.fields.echoon = 0;
	return 1;
}


int cmd_sleep (parser_t *parser) {
	int ms;
	if (!parser_expression(parser, &ms) ) {
		console_printf(syntax_error, not_an_expression);
		return 0;
	}
	if (ms < 0 || ms > 3600000) { // max 1 hour
		console_printf(invalid_val, ms);
		return 0;
	}
	HAL_Delay(ms);
	return 1;
}


int cmd_print (parser_t *parser) {
	int n;
	if (!parser_expression(parser, &n) ) {
		console_printf(syntax_error, not_an_expression);
		return 0;
	}
	console_printf("%i", n);
	return 1;
}







int cmd_savecfg (parser_t *parser) {
	int rc = save_devicecfg();

	if (rc && config.fields.echoon) {
		console_printf("%i bytes", rc);
	}
	if (config.fields.echoon) {
		console_printf("cfg save %s", rc ? "success" : "error");
	}
	return rc;
}

int cmd_loadcfg (parser_t *parser) {
	int rc = load_devicecfg();

	if (rc && config.fields.echoon) {
		console_printf("%i bytes", rc);
	}
	if (config.fields.echoon) {
		console_printf("cfg load %s", rc ? "success" : "error");
	}
	return rc;
}


int cmd_saveprg (parser_t *parser) {
	int entry;
	if (!parser_expression(parser, &entry)) {
		console_printf(syntax_error, not_an_expression);
		return 0;
	}
	if (entry >= direntries()) {
		console_printf("'%i' doesn't exist", entry);
		return 0;
	}
	int rc = program_save(program, directory[entry].file);
	if (rc && config.fields.echoon) {
		console_printf("%i bytes", rc);
	}
	if (config.fields.echoon) {
		console_printf("prg save %s", rc ? "success" : "error");
	}
	return rc;
}

int cmd_loadprg (parser_t *parser) {
	int entry;
	if (!parser_expression(parser, &entry)) {
		console_printf(syntax_error, not_an_expression);
		return 0;
	}
	if (entry >= direntries()) {
		console_printf("'%i' doesn't exist", entry);
		return 0;
	}
	int rc = program_load(program, directory[entry].file);
	if (rc && config.fields.echoon) {
		console_printf("%i bytes", rc);
	}
	if (config.fields.echoon) {
		console_printf("prg load %s", rc ? "success" : "error");
	}
	return rc;
}

int cmd_program_new (parser_t *parser) {
	char* endstr = " ";
	char* line;

	for (int i = 0; i != program->header.fields.nlines; i++) {
		line = program_line(program, i);
		strcpy(line, endstr);
	}
	return 1;
}


int cmd_program_list (parser_t *parser) {
	char *line;
	for (int i = 0; i != program->header.fields.nlines; i++) {
		line = program_line(program, i);
		console_printf_e("%2i \"", i);
		for (int b = 0; b != program->header.fields.linelen; b++) {
			char byte = line[b];
			if (!byte) {
				break;
			}
			if (byte == '\"' || byte == '\'') {
				console_printf_e("\\");
			}
			console_printf_e("%c", byte);
		};
		console_printf("\"");
	}
	return 1;
}


int cmd_program_end (parser_t *parser) {
	program_run = 0;
	console_printf("Done");
	return 1;
}

int cmd_program_run (parser_t *parser) {
	execute_program(program);
	return 1;
}


int cmd_program_goto (parser_t *parser) {
	int line;
	if (!parser_expression(parser, &line) ) {
		console_printf(syntax_error, not_an_expression);
		return 0;
	}
	if (line < 0 || line > 15) {
		console_printf(invalid_val, line);
		return 0;
	}
	program_ip = line;
	return 1;
}


int cmd_program_gosub (parser_t *parser) {
	int line;
	if (!parser_expression(parser, &line) ) {
		console_printf(syntax_error, not_an_expression);
		return 0;
	}
	if (line < 0 || line > 15) {
		console_printf(invalid_val, line);
		return 0;
	}
	subroutine_stack[subroutine_sp] = program_ip; // At this point the executing function has already increased the line number
	subroutine_sp += 1; // TODO error handling
	program_ip = line;
	return 1;
}

int cmd_program_return (parser_t *parser) {
	if (!subroutine_sp) {
		console_printf("Not in a subroutine");
		return 0;
	}
	subroutine_sp -= 1;
	program_ip = subroutine_stack[subroutine_sp];
	return 1;
}

int cmd_help (parser_t *parser);

_keyword_t keywords[] = {
		{"help", "- print this help", cmd_help},
		{"[0-16]", "\"cmdline\" - enter command line", NULL},
		{"new", "- clear program", cmd_program_new},
		{"end", "- end program", cmd_program_end},
		{"list", "- list program", cmd_program_list},
		{"run", "- run program", cmd_program_run},
		{"goto", "[line] - jump", cmd_program_goto},
		{"gosub", "[line] - call", cmd_program_gosub},
		{"return", "- return", cmd_program_return},
		{"if", "[expr] \"cmdline\" - execute cmdline if expr is true", parser_if},

		{"rfon", "- RF on", cmd_rfon},
		{"rfoff", "- RF off", cmd_rfoff},
		{"cfg", "- show cfg", cmd_show_cfg},

		{"echoon", "- Echo on", cmd_echoon},
		{"echooff", "- echo off", cmd_echooff},

		{"loadprg", "[n] - load program from file [n]", cmd_loadprg},
		{"saveprg", "[n] - save program to file [n]", cmd_saveprg},

		{"loadcfg", "- load config", cmd_loadcfg},
		{"savecfg", "- save config", cmd_savecfg},

		{"eer", "[page] - peek EEPROM", cmd_eeprom_read},
		{"sleep", "[millisecs] - sleep", cmd_sleep},
		{"print", "[expr] - print the value", cmd_print},

};

int cmd_help (parser_t *parser) {
	for (int i = 0; i != (sizeof(keywords)/sizeof(keywords[0])); i++) {
		console_printf("%s %s", keywords[i].token, keywords[i].helpstr);
	}
	return 1;
}

//================================================================================================


// this is how the (portable) lex reads a new byte in this type of parser
int parser_lex_read (lex_instance_t *instance, int *b) {
	char byte;
	parser_t *parser = (parser_t*)instance->context; // context of lex is the parser
	if (parser->cmd_op >= parser->line_length || parser->cmd_op >= parser->cmd_ip) {
		return 0;
	}
	byte = parser->cmdbuf[parser->cmd_op++];
	*b = byte;

	if (byte == (char)EOF) {
		return 0;
	}
	return 1;
}

// this is how lex prints an error message on this system
void parser_lex_error (lex_instance_t *instance, const char *str) {
	parser_t *parser = (parser_t*)instance->context; // context of lex is the parser
	console_printf("lex error: %s, \"%s\"", str, parser->lex->lexeme);
}


void parser_reset (parser_t *parser) {
	parser->cmd_ip = 0;
	parser->cmd_op = 0;
	parser->cmdbuf[parser->cmd_ip] = EOF;
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
	parser_reset(instance);
	instance->lex = lex_create(instance, instance->line_length, parser_lex_read, parser_lex_error, 0x00); // context of lex is the parser
	if (!instance->lex) {
		free(instance);
	    return NULL;
	}
	parser_reset(instance); // lex_create() automaticaly runs lex_reset()->next_token()->read_byte(), hence giving another reset
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

// new byte
int parser_fill (parser_t *parser, char b) {
	int rc = 0;
	parser->cmdbuf[parser->cmd_ip++] = b;
	if (parser->cmd_ip >= (parser->line_length - 1)) {
		parser->cmd_ip = (parser->line_length - 1);
		rc = 1;
	}
	parser->cmdbuf[parser->cmd_ip] = EOF;
	return rc;
}

// backspace
int parser_back (parser_t *parser) {
	int rc = 0;
	parser->cmd_ip -= 1;
	if (parser->cmd_ip < 0) {
		parser->cmd_ip = 0;
		rc = 1;
	}
	parser->cmdbuf[parser->cmd_ip] = EOF;
	return rc;
}

// run parser
int parser_run (parser_t *parser) {
	int rc = 1;

	if (!parser->cmd_ip) {
		return 1;
	}

	lex_reset(parser->lex);
	do {
		if (parser->lex->token == T_IDENTIFIER) {
			int i;

			for (i = 0; i != (sizeof(keywords)/sizeof(keywords[0])); i++) {
				if (!strcmp(parser->lex->lexeme, keywords[i].token)) {
					if (!keywords[i].exec) {
						continue;
					}
					next_token(parser->lex); //
					rc = keywords[i].exec(parser);
					if (!rc) {
						console_printf("%s %s", keywords[i].token, keywords[i].helpstr);
					}
					break;
				}
			}
			if (i == (sizeof(keywords)/sizeof(keywords[0]))) { // keywords exhausted, let's try assignment
				resource_t* resource = locate_resource(parser->lex->lexeme);
				if (resource) {
					next_token(parser->lex);
					if (!parser_assignment(parser, resource)) {
						console_printf("assignment expected");
						rc = 0;
					}
				} else {
					console_printf("Bad cmd \'%s\'", parser->lex->lexeme);
					rc = 0;
				}
			}
		} else if (parser->lex->token == T_INTEGER) { // edit a program line
			int nline = integer_value(parser->lex);
			if (nline < 0 || nline > 15) {
				console_printf("Bad line \'%i\'", nline);
				continue;
			}
			next_token(parser->lex);
			char* line = program_line(program, nline);

			char* cmdstr;
			if (!parser_string(parser, &cmdstr)) {
				console_printf("\"cmd\" expected");
				continue;
			}
			strcpy(line, cmdstr);
			next_token(parser->lex);
			if (config.fields.echoon) {
				cmd_program_list(parser);
			}
		}

	} while (lex_get(parser->lex, T_SEMICOLON, NULL));

	parser_reset(parser);
	return rc;
}


