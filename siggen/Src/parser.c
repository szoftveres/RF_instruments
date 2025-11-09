#include <parser.h>
#include "functions.h"
#include <string.h> // strcmp
#include <stdlib.h> //malloc
#include "instances.h"
#include "resource.h"
#include "stm32f4xx_hal.h" // HAL_Delay


static const char* invalid_val = "Invalid value \'%i\'";
static const char* not_a_string = "Not a string";
static const char* syntax_error = "Syntax error \'%s\'";
static const char* not_an_expression = "Not an expression";




int parser_expression (lex_instance_t *lex, int *n);
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



int parser_resource_expression (lex_instance_t *lex, int *n) {
	if (lex->token != T_IDENTIFIER) {
		return 0;
	}
	resource_t* resource = locate_resource(lex->lexeme);
	if (!resource) {
		return 0;
	}
	next_token(lex);
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



int parser_assignment (lex_instance_t *lex, resource_t *resource) {
	int n;

    if (lex_get(lex, T_ASSIGN, NULL)) {
        if (!parser_expect_expression(lex, &n)) {
        	return 0;
        }
        resource->set(resource->context, n);
        return 1;
    } else if (recursive_assignment(lex, resource->get(resource->context), &n)) {
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


int parser_primary_expression (lex_instance_t *lex, int *n) {
	int rc = 0;
	int neg = 0;

	if (lex->token == T_MINUS) {
		neg = 1;
		next_token(lex);
	}
    if (parser_parentheses(lex, n)) {
        rc = 1;
    } else if (parser_numeric_const(lex, n)) {
        rc = 1;
    } else if (parser_resource_expression(lex, n)) {
        rc = 1;
    } else if (parser_interactive_input_expression(lex, 40, n)) { // TODO linelen
    	rc = 1;
    }
    if (rc) {
    	*n *= (neg ? -1 : 1);
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


// Recursive "parser within parser"
int parse_str_cmd (parser_t *parser, char* cmdstr) {
	int rc = 1;
	parser_t *lcl_parser = parser_create(parser->line_length);
	if (!lcl_parser) {
		console_printf("malloc fail");
		return 0;
	}

	rc = cmd_line_parser(lcl_parser, cmdstr);

	parser_destroy(lcl_parser);
	return rc;
}


int parser_if (parser_t *parser) {
	int n;
	if (!parser_expect_expression(parser->lex, &n) ) {
		return 0;
	}
	if (!parser_string(parser->lex)) {
		console_printf(not_a_string);
		return 0;
	}
	if (!n) {
		return 1; // condition is false
	}
	if (!parse_str_cmd(parser, parser->lex->lexeme)) {
		return 0;
	}
	next_token(parser->lex);
	return 1;
}


int cmd_show_cfg (parser_t *parser) {
	print_cfg();
	return 1;
}


int cmd_sleep (parser_t *parser) {
	int ms;
	if (!parser_expect_expression(parser->lex, &ms) ) {
		return 0;
	}
	if (ms < 0 || ms > 3600000) { // max 1 hour
		console_printf(invalid_val, ms);
		return 0;
	}

	uint32_t tickstart = HAL_GetTick();

	while((HAL_GetTick() - tickstart) < ms) {
		if (switchstate()) {
			console_printf("Break");
			break;
		}
	}
	return 1;
}


int cmd_print (parser_t *parser) {
	int res;
	int rc = 0;

	int n;

	do {
		res = 0;

		if (parser_expression(parser->lex, &n) ) {
			console_printf_e("%i", n);
			res = 1;
		}
		if (parser_string(parser->lex) ) {
			console_printf_e("%s", parser->lex->lexeme);
			next_token(parser->lex);
			res = 1;
		}
		rc |= res;
	} while (res);

	if (rc) {
		console_printf("");
	}
	return rc;
}


int cmd_savecfg (parser_t *parser) {
	int rc = save_devicecfg();
	if (rc) {
		console_printf("%i bytes", rc);
	}
	console_printf("cfg save %s", rc ? "success" : "error");

	return rc;
}

int cmd_loadcfg (parser_t *parser) {
	int rc = load_devicecfg();
	if (rc) {
		console_printf("%i bytes", rc);
	}
	console_printf("cfg load %s", rc ? "success" : "error");

	return rc;
}


int cmd_saveprg (parser_t *parser) {
	int fd;
	int rc;

	if (!parser_string(parser->lex)) {
		console_printf("\"name\" expected");
		return 0;
	}

	fd = fs_open(eepromfs, parser->lex->lexeme, FS_O_CREAT | FS_O_TRUNC);
	next_token(parser->lex);

	if (fd < 0) {
		console_printf("open fail");
		return 0;
	}
	rc = program_save(program, eepromfs, fd);

	fs_close(eepromfs, fd);
	if (rc > 0) {
		console_printf("%i bytes", rc);
	}
	console_printf("prg save %s", rc > 0 ? "success" : "error");

	return rc;
}


int cmd_loadprg (parser_t *parser) {
	int fd;
	int rc;

	if (!parser_string(parser->lex)) {
		console_printf("\"name\" expected");
		return 0;
	}

	fd = fs_open(eepromfs, parser->lex->lexeme, 0);
	next_token(parser->lex);
	if (fd < 0) {
		console_printf("open fail");
		return 0;
	}
	rc = program_load(program, eepromfs, fd);

	fs_close(eepromfs, fd);
	if (rc > 0) {
		console_printf("%i bytes", rc);
	}
	console_printf("prg load %s", rc > 0 ? "success" : "error");

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
	if (!parser_expect_expression(parser->lex, &line) ) {
		return 0;
	}
	if (line < 0 || line >= program->header.fields.nlines) {
		console_printf(invalid_val, line);
		return 0;
	}
	program_ip = line;
	return 1;
}


int cmd_program_gosub (parser_t *parser) {
	int line;
	if (!parser_expect_expression(parser->lex, &line) ) {
		return 0;
	}
	if (line < 0 || line >= program->header.fields.nlines) {
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


int cmd_vars (parser_t *parser) {
	for (resource_t* r = resource_it_start(); r; r = resource_it_next(r)) {
		console_printf("%s : %i", r->name, r->get(r));
	}
	return 1;
}


int cmd_ver (parser_t *parser) {
	console_printf("%s - %s", __DATE__ ,__TIME__);
	return 1;
}


int cmd_fsinfo (parser_t *parser) {
	int n;
	//fs_dump_fat(eepromfs);
	//console_printf("file_entry_t %i", sizeof(file_entry_t));
	//console_printf("device_params_t %i", sizeof(device_params_t));
	leading_wspace(0, 20);
	console_printf("%i entries free", fs_count_empyt_direntries(eepromfs));
	n = fs_count_empyt_blocks(eepromfs);
	leading_wspace(0, 20);
	console_printf("%i blocks (%i Bytes) free", n, (n * eepromfs->device->blocksize));
	return 1;
}


int cmd_format (parser_t *parser) {
	fs_format(eepromfs, 16);
	return cmd_fsinfo(parser);
}


int cmd_del (parser_t *parser) {
	int rc;
	if (!parser_string(parser->lex)) {
		console_printf("\"name\" expected");
		return 0;
	}
	rc = fs_delete(eepromfs, parser->lex->lexeme);
	next_token(parser->lex);
	if (rc < 0) {
		console_printf("delete fail");
	}

	return 1;
}


int cmd_dir (parser_t *parser) {
	int n = 0;
	file_entry_t *entry;

	while (1) {
		int nlen;
		entry = fs_walk_dir(eepromfs, &n);
		if (!entry) {
			break;
		}
		nlen = strlen(entry->name);
		console_printf_e("%s", entry->name);
		leading_wspace(nlen, 20);
		nlen = console_printf_e("%i", entry->size);
		leading_wspace(nlen, 20);
		console_printf("n:%02i,attr:0x%04x,start:0x%04x", n, entry->attrib, entry->start);
	}
	return cmd_fsinfo(parser);
}


int
cmd_hexdump (parser_t *parser) {
	char buf[16];
	int fd;
	int rc = 16;
    int i;
    int addr = 0;

	if (!parser_string(parser->lex)) {
		console_printf("\"name\" expected");
		return 0;
	}

	fd = fs_open(eepromfs, parser->lex->lexeme, 0);
	next_token(parser->lex);
	if (fd < 0) {
		console_printf("open fail");
		return 0;
	}

    while (rc == 16) {
    	rc = fs_read(eepromfs, fd, buf, 16);
        console_printf_e("%04X  ", addr);
        for (i = 0; i != 16; i++) {
        	if (i < rc) {
        		console_printf_e("%02X ", buf[i]);
        	} else {
        		console_printf_e("   ");
        	}
            if (i == 7) {
                console_printf_e(" ");
            }
        }
        console_printf_e(" |");
        for (i = 0; i != 16; i++) {
        	if (i < rc) {
				if (buf[i] < 0x20 || buf[i] > 0x7E) {
					console_printf_e(".");
				} else {
					console_printf_e("%c", buf[i]);
				}
        	} else {
        		console_printf_e(" ");
        	}
        }
        addr += 16;
        console_printf("|");
    }

	fs_close(eepromfs, fd);

    return 1;
}


/*
int cmd_fat (parser_t *parser) {
	fs_dump_fat(eepromfs);
	return 1;
}
*/


int cmd_rfon (parser_t *parser) {
	set_rf_output(1);
	print_cfg();
	return 1;
}


int cmd_rfoff (parser_t *parser) {
	set_rf_output(0);
	print_cfg();
	return 1;
}


int cmd_amtone (parser_t *parser) {
	int ms;
	if (!parser_expect_expression(parser->lex, &ms) ) {
		return 0;
	}
	if (ms < 0 || ms > 3600000) { // max 1 hour
		console_printf(invalid_val, ms);
		return 0;
	}

	uint32_t tickstart = HAL_GetTick();
	uint32_t thistick;
	int level = rflevel_getter(NULL);
	int state = 0;

	while(((thistick = HAL_GetTick()) - tickstart) < ms) {
		if (switchstate()) {
			console_printf("Break");
			break;
		}
		if (!set_rf_level(state ? -30 : level)) {
			break;
		}
		state += 1; state %= 2;
		while (thistick == HAL_GetTick());
	}

	return (set_rf_level(level));
}


int cmd_fmtone (parser_t *parser) {
	int ms;
	int dev;

	if (!parser_expect_expression(parser->lex, &dev) ) {
		return 0;
	}
	if (dev < 10 || dev > 1000) { // 10 kHz - 1 MHz
		console_printf(invalid_val, dev);
		return 0;
	}

	if (!parser_expect_expression(parser->lex, &ms) ) {
		return 0;
	}
	if (ms < 0 || ms > 3600000) { // max 1 hour
		console_printf(invalid_val, ms);
		return 0;
	}

	uint32_t tickstart = HAL_GetTick();
	uint32_t thistick;
	int freq = frequency_getter(NULL);
	int state = 0;

	while(((thistick = HAL_GetTick()) - tickstart) < ms) {
		if (switchstate()) {
			console_printf("Break");
			break;
		}
		if (!set_rf_frequency(freq + (state ? dev : -dev))) {
			break;
		}
		state += 1; state %= 2;
		while (thistick == HAL_GetTick());
	}

	return (set_rf_frequency(freq));
}


int cmd_help (parser_t *parser);

_keyword_t keywords[] = {
		{"help", "- print this help", cmd_help},
		{"vars", "- print rsrc vars", cmd_vars},
		{"ver", "- FW build", cmd_ver},

		{"format", "- format EEPROM", cmd_format},
		{"del", "\"file\" - del file", cmd_del},
		{"dir", "- list files", cmd_dir},
		{"hexdump", "\"file\"", cmd_hexdump},


		{"[0-n]", "\"cmdline\" - enter command line", NULL},
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

		{"loadprg", "\"name\" - load program", cmd_loadprg},
		{"saveprg", "\"name\" - save program", cmd_saveprg},

		{"loadcfg", "- load config", cmd_loadcfg},
		{"savecfg", "- save config", cmd_savecfg},

		{"amtone", " [ms] - AM tone", cmd_amtone},
		{"fmtone", " [dev] [ms] - FM tone", cmd_fmtone},

		{"sleep", "[millisecs] - sleep", cmd_sleep},
		{"print", "[expr] \"str\"", cmd_print},
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



int cmd_line_parser (parser_t *parser, char* line) {
	int rc = 1;

	if (!line) {
		return 0;
	}
	strcpy(parser->cmdbuf, line);

	parser->cmd_op = 0;
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
					if (rc < 1) {
						console_printf("%s %s", keywords[i].token, keywords[i].helpstr);
					}
					break;
				}
			}
			if (i == (sizeof(keywords)/sizeof(keywords[0]))) { // keywords exhausted, let's try assignment
				resource_t* resource = locate_resource(parser->lex->lexeme);
				if (resource) {
					next_token(parser->lex);
					if (!parser_assignment(parser->lex, resource)) {
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
			if (nline < 0 || nline >= program->header.fields.nlines) {
				console_printf("Bad line \'%i\'", nline);
				continue;
			}
			next_token(parser->lex);
			char* line = program_line(program, nline);

			if (!parser_string(parser->lex)) {
				console_printf("\"cmd\" expected");
				continue;
			}
			if ((strlen(parser->lex->lexeme) + 1) < program->header.fields.linelen) {
				strcpy(line, parser->lex->lexeme);
			} else {
				console_printf("too long");
			}
			next_token(parser->lex);
			cmd_program_list(parser);
		}

	} while (lex_get(parser->lex, T_SEMICOLON, NULL));

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
		if (!parser_expect_expression(parser->lex, n)) {
			rc = 0;
			break;
		}
	} while (lex_get(parser->lex, T_COMMA, NULL));

	return rc;
}

