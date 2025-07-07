#include <parser.h>
#include "functions.h"
#include <string.h> // strcmp
#include <stdlib.h> //malloc
#include <stdio.h> // EOF
#include "instances.h"
#include "eeprom.h"

#define PROGRAM_BASE_ADDRESS (0x100)


static const char* invalid_val = "Invalid value \'%i\'";
static const char* not_a_string = "Not a string \'%s\'";
static const char* syntax_error = "Syntax error \'%s\'";
static const char* not_an_expression = "Not an expression";


static int variables['z'-'a' + 1];


int parser_expression (parser_t *parser, int *n);
int parser_primary_expression (parser_t *parser, int *n);


int parser_find_variable (parser_t *parser) {
	char lcl_var_name[] = {'_', 'a', '\0'};

	for (int i = 0; i != (sizeof(variables)/sizeof(variables[0])); i++) {
		lcl_var_name[1] = 'a' + i;
		if (lex_get(parser->lex, T_IDENTIFIER, lcl_var_name)) {
			return i;
		}
	}
	return -1;
}


int parser_variable_expression (parser_t *parser, int *n) {
	int varidx = parser_find_variable(parser);
	if (varidx < 0) {
		return 0;
	}
	*n = variables[varidx];
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



int recursive_assignment (parser_t *parser, int varidx, int *right) {
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

    int left = variables[varidx];

    if (!parser_expression(parser, right)) {    /* only one expression after assignment */
    	console_printf("expected expression after assignment operator");
    }
    if (!do_operations(op_type, &left, *right)) {
    	return 0;
    }
    *right = left;
    return 1;
}



int parser_assignment (parser_t *parser, int varidx) {
	int n;

    if (lex_get(parser->lex, T_ASSIGN, NULL)) {
        if (!parser_expression(parser, &n)) {
        	console_printf("expected expression after '='");
        }
        variables[varidx] = n;
        return 1;
    } else if (recursive_assignment(parser, varidx, &n)) {
    	variables[varidx] = n;
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
    if (parser_variable_expression(parser, n)) {
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



//==============================================================



typedef struct {
	char* token;
	char* helpstr;
	int (*exec) (parser_t*);
} cmd_t;



int cmd_eeprom_read (parser_t *parser) {
	uint8_t b;
	int addr;
	int lcl_addr;
	if (!parser_expression(parser, &addr) ) {
		console_printf(syntax_error, not_an_expression);
		return 0;
	}
	console_printf_e("%04x | ", addr);

	lcl_addr = addr;
	for (int i = 0; i != 16; i++) {
		b = eeprom_read_byte((uint16_t)lcl_addr);
		console_printf_e("%02x ", b);
		lcl_addr++;
	}/*
	console_printf_e("| ");
	lcl_addr = addr;
	for (int i = 0; i != 16; i++) {
		b = eeprom_read_byte((uint16_t)lcl_addr);
		console_printf_e("%c", b);
		lcl_addr++;
	}*/
	console_printf(" |");
	return 1;
}


int cmd_show_cfg (parser_t *parser) {
	console_printf("RF: %i kHz, %i dBm, output %s", config.fields.khz, config.fields.level, config.fields.rfon ? "on" : "off");
	return 1;
}


int cmd_save_state (parser_t *parser) {
	validate_config(&config);
	uint16_t eeprom_address = EEPROM_CONFIG_ADDRESS;
	uint8_t *conf_p = (uint8_t*)&config;
	int bytes = 0;
	for (int i = 0; i < sizeof(config_t); conf_p += bytes, i += bytes, eeprom_address += bytes) {
		bytes = eeprom_write_page(eeprom_address, conf_p);
	}
	console_printf("Config saved");
	return 1;
}


int cmd_load_cfg (parser_t *parser) {
	config_t loadcfg;
	uint16_t eeprom_address = EEPROM_CONFIG_ADDRESS;
	uint8_t *conf_p = (uint8_t*)&loadcfg;
	for (int i = 0; i != sizeof(config_t); i++) {
		*conf_p = eeprom_read_byte(eeprom_address);
		conf_p++;
		eeprom_address++;
	}
	if (!verify_config(&loadcfg)) {
		memcpy(&config, &loadcfg, sizeof(config_t));
	}

	config.fields.rfon = 1; // Always start up with RF on
	validate_config(&config);
	set_rf_frequency(config.fields.khz);
	set_rf_level(config.fields.level);
	set_rf_output(config.fields.rfon);
	return cmd_show_cfg(parser);
}


int load_config (void) {
	return cmd_load_cfg(NULL);
}


int cmd_set_freq (parser_t *parser) {
	int set_khz;
	double khz;
	if (!parser_expression(parser, &set_khz) ) {
		console_printf(syntax_error, not_an_expression);
		return 0;
	}
	khz = set_rf_frequency(set_khz);
	if (khz < 0) {
		console_printf(invalid_val, set_khz);
		return 0;
	}
	console_printf("actual: %i.%03i kHz", (int)khz, (int)(khz * 1000.0) % 1000);
	return cmd_show_cfg(parser);
}


int cmd_set_rflevel (parser_t *parser) {
	int dBm;
	if (!parser_expression(parser, &dBm) ) {
		console_printf(syntax_error, not_an_expression);
		return 0;
	}
	if (!set_rf_level(dBm)) {
		console_printf(invalid_val, dBm);
		return 0;
	}
	return cmd_show_cfg(parser);
}


int cmd_rfon (parser_t *parser) {
	set_rf_output(1);
	return cmd_show_cfg(parser);
}


int cmd_rfoff (parser_t *parser) {
	set_rf_output(0);
	return cmd_show_cfg(parser);
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


// Recursive "parser within parser"
int parse_str (parser_t *parser) {
	int rc = 1;
	str_value(parser->lex); // CMD in lexeme
	parser_t *lcl_parser = parser_create(parser->line_length);
	if (!lcl_parser) {
		console_printf("malloc fail");
		return 0;
	}
	// copying the command string to the new parser
	char* lp = parser->lex->lexeme;
	while (*lp) {
		parser_fill(lcl_parser, *lp++);
	}
	parser_fill(lcl_parser, EOF);

	rc = parser_run(lcl_parser);

	parser_destroy(lcl_parser);
	return rc;
}



int cmd_exec (parser_t *parser) {
	if (parser->lex->token != T_STRING) {
		console_printf(not_a_string, parser->lex->lexeme);
		return 0;
	}
	if (!parse_str(parser)) {
		return 0;
	}
	next_token(parser->lex);
	return 1;
}


int cmd_program_new (parser_t *parser) {
	char* endstr = "end";
	int line;

	for (line = 0; line != 16; line++) {
		eeprom_write_page(PROGRAM_BASE_ADDRESS + (parser->line_length * line), (uint8_t*)endstr);
	}
	return 1;
}


int cmd_program_list (parser_t *parser) {
	int ip;

	for (ip = 0; ip != 16; ip++) {
		int line_addr = PROGRAM_BASE_ADDRESS + (parser->line_length * ip);
		// reading command from EEPROM into the new parser, EOF at the end
		uint8_t byte;
		console_printf_e("%2i  \" ", ip);
		while (1) {
			byte = eeprom_read_byte(line_addr++);
			if (!byte) {
				break;
			}
			if (byte == '\"' || byte == '\'') {
				console_printf_e("\\");
			}
			console_printf_e("%c", byte);
		};
		console_printf(" \"", ip);
	}
	return 1;
}


int cmd_program_end (parser_t *parser) {
	program_run = 0;
	return 1;
}

int cmd_program_run (parser_t *parser) {
	int rc = 1;
	program_ip = 0;
	program_run = 1;

	while (program_run) {
		int line_addr = PROGRAM_BASE_ADDRESS + (parser->line_length * program_ip);
		program_ip += 1;

		parser_t *lcl_parser = parser_create(parser->line_length);
		if (!lcl_parser) {
			program_run = 0;
			rc = 0;
			break;
		}

		// reading command from EEPROM into the new parser, EOF at the end
		uint8_t byte;
		while ((byte = eeprom_read_byte(line_addr++))) {
			parser_fill(lcl_parser, byte);
		}
		parser_fill(lcl_parser, EOF);

		rc = parser_run(lcl_parser);
		if (!rc) {
			program_run = 0;
		}

		parser_destroy(lcl_parser);

		if (program_ip > 15) {
			console_printf("program STOP");
			program_run = 0;
			rc = 0;
			break;
		}
	}
	return rc;
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


int cmd_help (parser_t *parser);

cmd_t commands[] = {
		{"help", "- print this help", cmd_help},
		{"freq", "[kHz] - set freq", cmd_set_freq},
		{"level", "[dBm] - set RF level", cmd_set_rflevel},
		{"rfon", "- RF on", cmd_rfon},
		{"rfoff", "- RF off", cmd_rfoff},
		{"cfg", "- show cfg", cmd_show_cfg},
		{"load", "- reload cfg", cmd_load_cfg},
		{"save", "- save cfg", cmd_save_state},
		{"eer", "[0xADDR] - peek EEPROM", cmd_eeprom_read},
		{"sleep", "[millisecs] - sleep", cmd_sleep},
		{"print", "[expr] - print the value", cmd_print},
		{"exec", "\"cmdline\" - execute cmdline", cmd_exec},
		{"[0-16]", "\"cmdline\" - edit command line", NULL},

		{"new", "- clear program", cmd_program_new},
		{"end", "- end program", cmd_program_end},
		{"list", "- list program", cmd_program_list},
		{"run", "- run program", cmd_program_run},
		{"goto", "[line] - jump", cmd_program_goto},
};

int cmd_help (parser_t *parser) {
	for (int i = 0; i != (sizeof(commands)/sizeof(commands[0])); i++) {
		console_printf("%s %s", commands[i].token, commands[i].helpstr);
	}
	return 1;
}

//================================================================================================


// this is how the (portable) lex reads a new byte in this type of parser
int parser_lex_read (lex_instance_t *instance, int *b) {
	char byte;
	parser_t *parser = (parser_t*)instance->context; // context of lex is the parser
	if (parser->cmd_op >= CMD_LEN || parser->cmd_op >= parser->cmd_ip) {
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
	instance->lex = lex_create(instance, CMD_LEN, parser_lex_read, parser_lex_error, 0x00); // context of lex is the parser
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
void parser_fill (parser_t *parser, char b) {
	parser->cmdbuf[parser->cmd_ip++] = b;
	if (parser->cmd_ip >= (CMD_LEN - 1)) {
		parser->cmd_ip = (CMD_LEN - 1);
	}
	parser->cmdbuf[parser->cmd_ip] = EOF;
}

// backspace
void parser_back (parser_t *parser) {
	parser->cmd_ip -= 1;
	if (parser->cmd_ip < 0) {
		parser->cmd_ip = 0;
	}
	parser->cmdbuf[parser->cmd_ip] = EOF;
}

// run parser
int parser_run (parser_t *parser) {
	int rc = 1;
	int varidx;

	if (!parser->cmd_ip) {
		return 1;
	}

	lex_reset(parser->lex);
	do {
		if (parser->lex->token == T_IDENTIFIER) {
			int i;

			for (i = 0; i != (sizeof(commands)/sizeof(commands[0])); i++) {
				if (!strcmp(parser->lex->lexeme, commands[i].token)) {
					if (!commands[i].exec) {
						continue;
					}
					next_token(parser->lex); //
					rc = commands[i].exec(parser);
					if (!rc) {
						console_printf("%s %s", commands[i].token, commands[i].helpstr);
					}
					break;
				}
			}
			if (i == (sizeof(commands)/sizeof(commands[0]))) {
				if ((varidx = parser_find_variable(parser)) >= 0) {
					//next_token(parser->lex); // XXX this shouldn't be needed
					if (!parser_assignment(parser, varidx)) {
						console_printf("assignment expected");
						rc = 0;
					}
				} else {
					console_printf("Bad cmd \'%s\'", parser->lex->lexeme);
					rc = 0;
				}
			}
		} else if (parser->lex->token == T_INTEGER) { // edit a program line
			int line = integer_value(parser->lex);
			if (line < 0 || line > 15) {
				console_printf("Bad line \'%i\'", line);
				continue;
			}
			next_token(parser->lex);

			int line_addr = PROGRAM_BASE_ADDRESS + (parser->line_length * line);
			if (parser->lex->token != T_STRING) {
				console_printf("\"cmd\" expected");
				continue;
			}
			str_value(parser->lex);
			for (int cp = 0; cp < parser->line_length ; cp += EEPROM_PAGE_SIZE) {
				eeprom_write_page(line_addr + cp, ((uint8_t*)parser->lex->lexeme) + cp);
			}
			next_token(parser->lex);

			// TODO: list the program after each line edit
		}

	} while (lex_get(parser->lex, T_SEMICOLON, NULL));

	parser_reset(parser);
	return rc;
}


