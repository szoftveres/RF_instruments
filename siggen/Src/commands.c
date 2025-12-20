#include "keyword.h"
#include "instances.h"
#include "functions.h"
#include "parser.h"
#include "resource.h"
#include "keyword.h"
#include <stddef.h> //NULL
#include <string.h> //strcpy
#include <stdlib.h> //malloc/free
#include "stm32f4xx_hal.h" // HAL_Delay


static const char* invalid_val = "Invalid value \'%i\'";
static const char* not_a_string = "Not a string";
static const char* name_expected = "\"name\" expected";
static const char* malloc_fail = "out of memory";


// Recursive "parser within parser"
int parse_str_cmd (parser_t *parser, char* cmdstr) {
	int rc = 1;
	parser_t *lcl_parser = parser_create(parser->line_length);
	if (!lcl_parser) {
		console_printf(malloc_fail);
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
		if (switchbreak()) {
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
		console_printf(name_expected);
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
		console_printf(name_expected);
		return 0;
	}

	fd = fs_open(eepromfs, parser->lex->lexeme, FS_O_READONLY);
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
	char* endstr = "";
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
	char* line = terminal_get_line(online_input, " type \"yes\"> ", 1);
	if (strcmp(line, "yes")) {
		return 1;
	}
	fs_format(eepromfs, 16);
	return cmd_fsinfo(parser);
}


int cmd_del (parser_t *parser) {
	int rc;
	if (!parser_string(parser->lex)) {
		console_printf(name_expected);
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
		console_printf(name_expected);
		return 0;
	}

	fd = fs_open(eepromfs, parser->lex->lexeme, FS_O_READONLY);
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


int cmd_copy (parser_t *parser) {
	int fdsrc;
	int fdnew;
	int totalbytes = 0;
	void* buf;

	buf = malloc(eepromfs->device->blocksize);
	if (!buf) {
	    console_printf(malloc_fail);
	    return 0;
	}

	if (!parser_string(parser->lex)) {
		console_printf(name_expected);
		free(buf);
		return 0;
	}

	fdsrc = fs_open(eepromfs, parser->lex->lexeme, FS_O_READONLY);
	next_token(parser->lex);
	if (fdsrc < 0) {
		console_printf("open fail");
		free(buf);
		return 0;
	}

	if (!parser_string(parser->lex)) {
		console_printf(name_expected);
		fs_close(eepromfs, fdsrc);
		free(buf);
		return 0;
	}

	fdnew = fs_open(eepromfs, parser->lex->lexeme, FS_O_CREAT | FS_O_TRUNC);
	next_token(parser->lex);

	if (fdnew < 0) {
		console_printf("open fail");
		fs_close(eepromfs, fdsrc);
		free(buf);
		return 0;
	}

	int b;
	while ((b = fs_read(eepromfs, fdsrc, buf, eepromfs->device->blocksize)) > 0) {
		if (fs_write(eepromfs, fdnew, buf, b) != b) {
			console_printf("disk full");
			break;
		}
		totalbytes += b;
	}

	fs_close(eepromfs, fdnew);
	fs_close(eepromfs, fdsrc);
	free(buf);

	console_printf("%i bytes copied", totalbytes);

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

	while (((thistick = HAL_GetTick()) - tickstart) < ms) {
		if (switchbreak()) {
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

	while (((thistick = HAL_GetTick()) - tickstart) < ms) {
		if (switchbreak()) {
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


int cmd_help (parser_t *parser) {
	keyword_t *kw = keyword_it_start();
	while (kw) {
		console_printf("%s %s", kw->token, kw->helpstr);
		kw = keyword_it_next(kw);
	}
	return 1;
}


int cmd_alias (parser_t *parser) {
	char *aliasname;
	char *aliascmd;
	if (!parser_string(parser->lex)) {
		console_printf(not_a_string);
		return 0;
	}
	aliasname = strdup(parser->lex->lexeme);
	next_token(parser->lex);

	if (parser->lex->token != T_STRING) {
		console_printf(not_a_string);
		free(aliasname);
		return 0;
	}
	aliascmd = strdup(parser->lex->lexeme);
	next_token(parser->lex);

	keyword_add(aliasname, aliascmd, NULL);
	return 1;
}


int cmd_unalias (parser_t *parser) {
	keyword_t *kw;
	int rc = 0;

	if (!parser_string(parser->lex)) {
		console_printf(not_a_string);
		return 0;
	}
	/* TODO check if it was an alias (i.e. not a regular cmd) */
	kw = keyword_remove(parser->lex->lexeme);
	next_token(parser->lex);

	if (kw) {
		free(kw->token);
		free(kw->helpstr);
		rc = 1;
	}

	return rc;
}


int setup_commands (void) {

	//keyword_add("unalias", " - name", cmd_unalias);
	//keyword_add("alias", " - name \"commands\"", cmd_alias);


	// RF GENERATOR ==================================
	keyword_add("rfoff", "- RF off", cmd_rfoff);
	keyword_add("rfon", "- RF on", cmd_rfon);
	keyword_add("fmtone", " [dev] [ms] - FM tone", cmd_fmtone);
	keyword_add("amtone", " [ms] - AM tone", cmd_amtone);


	// CONFIG ==================================
	keyword_add("cfg", "- print cfg", cmd_show_cfg);
	keyword_add("savecfg", "- save config", cmd_savecfg);
	keyword_add("loadcfg", "- load config", cmd_loadcfg);


	// EEPROM FAT ==================================
	keyword_add("format", "- format EEPROM", cmd_format);
	keyword_add("del", "\"file\" - del file", cmd_del);
	keyword_add("hexdump", "\"file\"", cmd_hexdump);
	keyword_add("dir", "- list files", cmd_dir);
	keyword_add("copy", "\"src\" \"new\"", cmd_copy);


	// PROGRAM ==================================
	keyword_add("saveprg", "\"name\" - save program", cmd_saveprg);
	keyword_add("loadprg", "\"name\" - load program", cmd_loadprg);
	keyword_add("return", "- return", cmd_program_return);
	keyword_add("gosub", "[line] - call", cmd_program_gosub);
	keyword_add("goto", "[line] - jump", cmd_program_goto);
	keyword_add("end", "- end program", cmd_program_end);
	keyword_add("run", "- run program", cmd_program_run);
	keyword_add("list", "- list program", cmd_program_list);
	keyword_add("[0-n]", "\"cmdline\" - enter command line", NULL);
	keyword_add("new", "- clear program", cmd_program_new);


	// BASIC FUNCTIONS ==================================
	keyword_add("if", "[expr] \"cmdline\" - execute cmdline if expr is true", parser_if);
	keyword_add("sleep", "[millisecs] - sleep", cmd_sleep);
	keyword_add("print", "[expr] \"str\"", cmd_print);
	keyword_add("ver", "- FW build", cmd_ver);
	keyword_add("vars", "- print rsrc vars", cmd_vars);
	keyword_add("help", "- print this help", cmd_help);

	return 0;
}
