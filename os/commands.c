#include "globals.h"
#include "hal_plat.h"  // t_malloc
#include "parser.h"  // lcl parsers
#include "resource.h"
#include "keyword.h"
#include "dsp_maths.h" // sin, cos, isqrt
#include <stddef.h> //NULL
#include <string.h> //strcpy


static const char* invalid_val = "Invalid value \'%i\'\n";
static const char* not_a_number = "Not a number\n";
static const char* not_a_string = "Not a string\n";
static const char* name_expected = "\"name\" expected\n";
static const char* malloc_fail = "out of memory\n";


/* Builtin Functions */


int ticks_func (cmd_context_s* ctxt) {
    obj_add_num(&(ctxt->ret), ticks_getter());
    return 1;
}


int rnd_func (cmd_context_s* ctxt) {
    if (get_data_obj_type(ctxt->params) == OBJ_TYPE_NUM) {
        rnd_setter(ctxt->params->n);
        obj_consume(&(ctxt->params));
    } else {
        obj_add_num(&(ctxt->ret), rnd_getter());
    }
    return 1;
}


int sqrt_func (cmd_context_s* ctxt) {
    int n;

    if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
        printf_f(STDERR, not_a_number);
        return 0;
    }
    n = ctxt->params->n;
    obj_consume(&(ctxt->params));
	obj_add_num(&(ctxt->ret), isqrt(n));
    return 1;
}


int sine_func (cmd_context_s* ctxt) {
	int n;
	int samples;

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		printf_f(STDERR, not_a_number);
		return 0;
	}
	n = ctxt->params->n;
	obj_consume(&(ctxt->params));

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		printf_f(STDERR, not_a_number);
		return 0;
	}
	samples = ctxt->params->n;
	obj_consume(&(ctxt->params));

	obj_add_num(&(ctxt->ret), sin_func(n, samples));
	return 1;
}


int cosine_func (cmd_context_s* ctxt) {
    int n;
    int samples;

    if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
        printf_f(STDERR, not_a_number);
        return 0;
    }
    n = ctxt->params->n;
    obj_consume(&(ctxt->params));

    if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
        printf_f(STDERR, not_a_number);
        return 0;
    }
    samples = ctxt->params->n;
    obj_consume(&(ctxt->params));

    obj_add_num(&(ctxt->ret), cos_func(n, samples));
    return 1;
}


int spc_func (cmd_context_s* ctxt) {
	int linelen = program->header.fields.linelen;
	int n;

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		printf_f(STDERR, not_a_number);
		return 0;
	}
	n = ctxt->params->n;
	obj_consume(&(ctxt->params));
	if (n >= (linelen -1)) {
		printf_f(STDERR, "Too long\n");
		return 0;
	}

	char *s = (char*)t_malloc(linelen);
	int i;
	for (i = 0; i != n; i++) {
		s[i] = ' ';
	}
	s[i] = '\0';

	obj_add_str(&(ctxt->ret), s);
	t_free(s);
	return 1;
}


static int
fmt_putu (unsigned int num, int digits, char* output, int* op) {
    int n = 1;
    if (num / 10){
        n += fmt_putu(num / 10, digits ? (digits - 1) : 0, output, op);
    } else {
        while (digits) {
            output[(*op)++] = ' '; output[*op] = '\0';
            --digits;
            ++n;
        }
    }
    output[(*op)++] = (num % 10) + '0'; output[*op] = '\0';
    return n;
}


static int
fmt_putx (unsigned int num, int digits, char* output, int* op) {
    int n = 1;
    if (num / 0x10) {
        n += fmt_putx(num / 0x10, digits ? (digits - 1) : 0, output, op);
    } else {
        while (digits) {
            output[(*op)++] = '0'; output[*op] = '\0';
            --digits;
            ++n;
        }
    }
    output[(*op)++] = (num % 0x10) + (((num % 0x10) > 9) ? ('a' - 10) : ('0')); output[*op] = '\0';
    return n;
}


int fmt_func (cmd_context_s* ctxt) {
	int digits = 0;
	int rc = 1;
	char* fmtstring;

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_STR) {
		printf_f(STDERR, not_a_string);
		return 0;
	}

    char* output = (char*)t_malloc(program->header.fields.linelen);
    int op = 0;
    output[op] = '\0';

	fmtstring = t_strdup(ctxt->params->str);
	obj_consume(&(ctxt->params));

    for (char *fmt = fmtstring ;*fmt && rc; fmt++) {
        if (*fmt != '%') {
            output[op++] = *fmt; output[op] = '\0';
            continue;
        }
        fmt++;
        while ((*fmt >= '0') && (*fmt <= '9')) {
            digits *= 10;
            digits += (*fmt - '0');
            fmt++;
        }
        switch (*fmt) {
          case 'c':
            if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
                printf_f(STDERR, not_a_number);
                rc = 0;
                break;
            }
            output[op++] = ctxt->params->n; output[op] = '\0';
            obj_consume(&(ctxt->params));
            break;
          case 's': {
            if (get_data_obj_type(ctxt->params) != OBJ_TYPE_STR) {
                printf_f(STDERR, not_a_string);
                    rc = 0;
                    break;
                }
                strcat(output, ctxt->params->str);
                op += strlen(ctxt->params->str);
                obj_consume(&(ctxt->params));
            }
            break;
          case 'x':
          case 'X':
            if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
                printf_f(STDERR, not_a_number);
                rc = 0;
                break;
            }
            fmt_putx(ctxt->params->n, digits ? (digits - 1) : 0, output, &op);
            obj_consume(&(ctxt->params));

            digits = 0;
            break;
          case 'd':
          case 'i':
          case 'u':
            if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
                printf_f(STDERR, not_a_number);
                rc = 0;
                break;
            }
            if ((*fmt == 'd') || (*fmt == 'i')) {
                if (ctxt->params->n < 0) {
                    output[op++] = '-'; output[op] = '\0';
                    ctxt->params->n *= -1;
                }
            }
            fmt_putu(ctxt->params->n, digits ? (digits - 1) : 0, output, &op);
            obj_consume(&(ctxt->params));
            digits = 0;
            break;
        }
    }

    obj_add_str(&(ctxt->ret), output);

	t_free(fmtstring);
	t_free(output);

	return rc;
}


/* Commands */

int cmd_prompt_expr (cmd_context_s* ctxt) {
    data_obj_t* obj = NULL;
    int linelen = program->header.fields.linelen;
    int bytes;
    char* prompt;

    if (get_data_obj_type(ctxt->params) != OBJ_TYPE_STR) {
        printf_f(STDERR, not_a_string);
        return 0;
    }
    prompt = t_strdup(ctxt->params->str);
    obj_consume(&(ctxt->params));
    parser_t* online_parser = parser_create(linelen); // align to the program line length

    while (1) {
        printf_f(STDERR, "%s > ", prompt);
        char *buf = online_parser->cmdbuf;

        bytes = read_f_line(fs, STDIN, buf, linelen-1);

        if (bytes < 1) {
            break;
        }
        if (buf[bytes-1] != '\n') {
            printf_f(STDERR, "line too long\n");
            break;
        }
        buf[bytes-1] = '\0';

        obj = expr_line_parser(online_parser);

        if (obj) {
            obj_insert_end(&(ctxt->ret), obj);
            break;
        }
    }
    parser_destroy(online_parser);
    t_free(prompt);
    return 1;
}



// Recursive "parser within parser"
int parse_str_cmd (cmd_context_s* ctxt, char* cmdstr, fifo_t* in, fifo_t* out) {
	int rc = 1;
	parser_t *lcl_parser = parser_create(strlen(cmdstr) + 2); // XXX
	if (!lcl_parser) {
		printf_f(STDERR, malloc_fail);
		return 0;
	}
    strcpy(lcl_parser->cmdbuf, cmdstr);
	rc = cmd_line_parser(lcl_parser, in, out);

	parser_destroy(lcl_parser);
	return rc;
}


int cmd_try (cmd_context_s* ctxt) {
    if (get_data_obj_type(ctxt->params) != OBJ_TYPE_STR) {
        printf_f(STDERR, not_a_string);
        return 0;
    }
    parse_str_cmd(ctxt, ctxt->params->str, ctxt->in, ctxt->out);
    obj_consume(&(ctxt->params));
    return 1;
}


int parser_if (cmd_context_s* ctxt) {
	int n;
    int rc = 1;

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		return 0;
	}
	n = ctxt->params->n;
	obj_consume(&(ctxt->params));

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_STR) {
		printf_f(STDERR, not_a_string);
		return 0;
	}
	if (n) {
	    if (!parse_str_cmd(ctxt, ctxt->params->str, ctxt->in, ctxt->out)) {
		    rc = 0;
        }
	}
	obj_consume(&(ctxt->params));
	return rc;
}


int cmd_print (cmd_context_s* ctxt) {
	int res;
	int rc = 0;

	do {
		res = 0;

		if (get_data_obj_type(ctxt->params) == OBJ_TYPE_NUM) {
			printf_f(STDOUT, "%i", ctxt->params->n);
			res = 1;
			obj_consume(&(ctxt->params));
		}

		if (get_data_obj_type(ctxt->params) == OBJ_TYPE_STR) {
			printf_f(STDOUT, "%s", ctxt->params->str);
			res = 1;
			obj_consume(&(ctxt->params));
		}
		rc |= res;
	} while (res);

	if (rc) {
		printf_f(STDOUT, "\n");
	}
	return rc;
}


int cmd_saveprg (cmd_context_s* ctxt) {
	int fd;
	int rc;

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_STR) {
		printf_f(STDERR, name_expected);
		return 0;
	}

	fd = open_f(fs, ctxt->params->str, FS_O_CREAT | FS_O_TRUNC);
	obj_consume(&(ctxt->params));

	if (fd < 0) {
		printf_f(STDERR, "open fail\n");
		return 0;
	}
	rc = program_save(program, fs, fd);

	close_f(fs, fd);
	if (rc > 0) {
		printf_f(STDERR, "%i bytes\n", rc);
	}
	printf_f(STDERR, "prg save %s\n", rc > 0 ? "success" : "error");

	return rc;
}


int cmd_loadprg (cmd_context_s* ctxt) {
	int fd;
	int rc;

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_STR) {
		printf_f(STDERR, name_expected);
		return 0;
	}

	fd = open_f(fs, ctxt->params->str, FS_O_READONLY);
	obj_consume(&(ctxt->params));
	if (fd < 0) {
		printf_f(STDERR, "open fail\n");
		return 0;
	}
	rc = program_load(program, fs, fd);

	close_f(fs, fd);
	if (rc > 0) {
		printf_f(STDERR, "%i bytes\n", rc);
	}
	printf_f(STDERR, "prg load %s\n", rc > 0 ? "success" : "error");

	return rc;
}


int cmd_program_new (cmd_context_s* ctxt) {
	char* endstr = "";
	char* line;

	for (int i = 0; i != program->header.fields.nlines; i++) {
		line = program_line(program, i);
		strcpy(line, endstr);
	}
	return 1;
}


int cmd_program_list (cmd_context_s* ctxt) {
	char *line;
	for (int i = 0; i != program->header.fields.nlines; i++) {
		line = program_line(program, i);
		printf_f(STDOUT, "%2i \"", i);
		for (int b = 0; b != program->header.fields.linelen; b++) {
			char byte = line[b];
			if (!byte) {
				break;
			}
			if (byte == '\"' || byte == '\'') {
				printf_f(STDOUT, "\\");
			}
			printf_f(STDOUT, "%c", byte);
		};
		printf_f(STDOUT, "\"\n");
	}
	return 1;
}


int cmd_program_end (cmd_context_s* ctxt) {
	program_run = 0;
	printf_f(STDERR, "Done\n");
	return 1;
}


int cmd_program_run (cmd_context_s* ctxt) {
	execute_program(program, ctxt->in, ctxt->out);
	return 1;
}


int cmd_program_goto (cmd_context_s* ctxt) {
	int line;
	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		return 0;
	}
	line = ctxt->params->n;
	obj_consume(&(ctxt->params));
	if (line < 0 || line >= program->header.fields.nlines) {
		printf_f(STDERR, invalid_val, line);
		return 0;
	}
	program_ip = line;
	return 1;
}


int cmd_program_gosub (cmd_context_s* ctxt) {
	int line;
	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		return 0;
	}
	line = ctxt->params->n;
	obj_consume(&(ctxt->params));
	if (line < 0 || line >= program->header.fields.nlines) {
		printf_f(STDERR, invalid_val, line);
		return 0;
	}
	subroutine_stack[subroutine_sp] = program_ip; // Already the next line
	subroutine_sp += 1; // TODO error handling
	program_ip = line;
	return 1;
}


int cmd_program_return (cmd_context_s* ctxt) {
	if (!subroutine_sp) {
		printf_f(STDERR, "Not in a subroutine\n");
		return 0;
	}
	subroutine_sp -= 1;
	program_ip = subroutine_stack[subroutine_sp];
	return 1;
}


int cmd_vars (cmd_context_s* ctxt) {
	for (resource_t* r = resource_it_start(); r; r = resource_it_next(r)) {
        printf_f(STDOUT, "%s : ", r->name);
        switch (r->obj->type) {
          case OBJ_TYPE_NUM:
            printf_f(STDOUT, "%i", r->obj->n);
            break;
          case OBJ_TYPE_STR:
            printf_f(STDOUT, "\"%s\"", r->obj->str);
            break;
          default:
        	break;
        }
        printf_f(STDOUT, "\n");
	}
	return 1;
}


int cmd_clr (cmd_context_s* ctxt) {
    resource_remove_all();
	return 1;
}


int cmd_ver (cmd_context_s* ctxt) {
	printf_f(STDOUT, "%s - %s\n", __DATE__ ,__TIME__);
	return 1;
}


int cmd_mem (cmd_context_s* ctxt) {
	printf_f(STDOUT, "chunks:%i\n", t_chunks());
	return 1;
}



int cmd_del (cmd_context_s* ctxt) {
	int rc;

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_STR) {
		printf_f(STDERR, name_expected);
		return 0;
	}
	rc = delete_f(fs, ctxt->params->str);
	obj_consume(&(ctxt->params));
	if (rc < 0) {
		printf_f(STDERR, "delete fail\n");
	}

	return 1;
}


int
cmd_change_fs (cmd_context_s* ctxt) {
	int rc;

    if (get_data_obj_type(ctxt->params) != OBJ_TYPE_STR) {
    	printf_f(STDERR, name_expected);
		return 0;
	}
    rc = change_current_fs(fs, ctxt->params->str[0]);
	obj_consume(&(ctxt->params));
	if (!rc) {
		printf_f(STDERR, "Invalid fs\n");
	}
	return rc;
}


void leading_wspace (int start, int stop) {
	for (int i = start; i < stop; i++) {
		printf_f(STDOUT, " ");
	}
}


int cmd_dir (cmd_context_s* ctxt) {

	int rc;
	char* name;
	int size;

	opendir_f(fs);
	while ((rc = walkdir_f(fs, &name, &size))) {
		int nlen;
		nlen = strlen(name);
		printf_f(STDOUT, "%s", name);
		leading_wspace(nlen, 20);
		nlen = printf_f(STDOUT, "%i", size);
		leading_wspace(nlen, 20);
		//printf_f(STDOUT, "attr:0x%04x,start:0x%04x\n", entry->attrib, entry->start);
		printf_f(STDOUT, "\n");
	}
	closedir_f(fs);
	return cmd_fsinfo();
}


int
cmd_hexdump (cmd_context_s* ctxt) {
	char buf[16];
	int fd;
	int rc = 16;
    int i;
    int addr = 0;

    if (get_data_obj_type(ctxt->params) != OBJ_TYPE_STR) {
    	printf_f(STDERR, name_expected);
		return 0;
	}

	fd = open_f(fs, ctxt->params->str, FS_O_READONLY);
	obj_consume(&(ctxt->params));
	if (fd < 0) {
		printf_f(STDERR, "open fail\n");
		return 0;
	}

    while (rc == 16) {
    	rc = read_f_all(fs, fd, buf, 16);
    	printf_f(STDOUT, "%04X  ", addr);
        for (i = 0; i != 16; i++) {
        	if (i < rc) {
        		printf_f(STDOUT, "%02X ", buf[i]);
        	} else {
        		printf_f(STDOUT, "   ");
        	}
            if (i == 7) {
            	printf_f(STDOUT, " ");
            }
        }
        printf_f(STDOUT, " |");
        for (i = 0; i != 16; i++) {
        	if (i < rc) {
				if (buf[i] < 0x20 || buf[i] > 0x7E) {
					printf_f(STDOUT, ".");
				} else {
					printf_f(STDOUT, "%c", buf[i]);
				}
        	} else {
        		printf_f(STDOUT, " ");
        	}
        }
        addr += 16;
        printf_f(STDOUT, "|\n");
    }

	close_f(fs, fd);

    return 1;
}

#define COPY_UNIT (32)   // XXX for now
int cmd_copy (cmd_context_s* ctxt) {
	int fdsrc;
	int fdnew;
	int copyunit = COPY_UNIT;
	int bytestocopy = -1;
	int totalbytes = 0;
	void* buf;

	buf = t_malloc(COPY_UNIT);
	if (!buf) {
		printf_f(STDERR, malloc_fail);
	    return 0;
	}

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_STR) {
		printf_f(STDERR, name_expected);
		t_free(buf);
		return 0;
	}

	fdsrc = open_f(fs, ctxt->params->str, FS_O_READONLY);
	obj_consume(&(ctxt->params));
	if (fdsrc < 0) {
		printf_f(STDERR, "src open fail\n");
		t_free(buf);
		return 0;
	}

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_STR) {
		printf_f(STDERR, name_expected);
		close_f(fs, fdsrc);
		t_free(buf);
		return 0;
	}

	fdnew = open_f(fs, ctxt->params->str, FS_O_CREAT | FS_O_TRUNC);
	obj_consume(&(ctxt->params));

	if (fdnew < 0) {
		printf_f(STDERR, "dst open fail\n");
		close_f(fs, fdsrc);
		t_free(buf);
		return 0;
	}

	if (get_data_obj_type(ctxt->params) == OBJ_TYPE_NUM) {
		bytestocopy = ctxt->params->n;
		obj_consume(&(ctxt->params));
	}


	int b;
	if (bytestocopy >= 0) {
		copyunit = bytestocopy;
	}
	while ((b = read_f_all(fs, fdsrc, buf, copyunit)) > 0) {
		if (write_f_all(fs, fdnew, buf, b) != b) {
			printf_f(STDERR, "disk full\n");
			break;
		}
		totalbytes += b;
		if ((bytestocopy >= 0) && (totalbytes >= bytestocopy)) {
			break;
		}
	}

	close_f(fs, fdnew);
	close_f(fs, fdsrc);
	t_free(buf);

	printf_f(STDERR, "\n%i bytes copied\n", totalbytes);

	return 1;
}


int cmd_help (cmd_context_s* ctxt) {
	keyword_t *kw = keyword_it_start();
	while (kw) {
		printf_f(STDOUT, "  %s %s\n", kw->token, kw->helpstr);
		kw = keyword_it_next(kw);
	}
	return 1;
}


int cmd_alias (cmd_context_s* ctxt) {
	char *aliasname;
	char *aliascmd;
	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_STR) {
		printf_f(STDERR, not_a_string);
		return 0;
	}
	aliasname = t_strdup(ctxt->params->str);
	obj_consume(&(ctxt->params));

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_STR) {
		printf_f(STDERR, not_a_string);
		t_free(aliasname);
		return 0;
	}
	aliascmd = t_strdup(ctxt->params->str);
	obj_consume(&(ctxt->params));

	keyword_add(aliasname, aliascmd, NULL);
	return 1;
}


int cmd_unalias (cmd_context_s* ctxt) {
	keyword_t *kw;
	int rc = 0;

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_STR) {
		printf_f(STDERR, not_a_string);
		return 0;
	}
	/* TODO check if it was an alias (i.e. not a regular cmd) */
	kw = keyword_remove(ctxt->params->str);
	obj_consume(&(ctxt->params));

	if (kw) {
		t_free(kw->token);
		t_free(kw->helpstr);
		rc = 1;
	}

	return rc;
}




int cmd_exit (cmd_context_s* ctxt) {
	return -1;
}



/* ================================================================== */


int setup_commands (void) {


	//keyword_add("unalias", " - name", cmd_unalias);
	//keyword_add("alias", " - name \"commands\"", cmd_alias);



	// BUILTIN FUNCTIONS =======================
	keyword_add("ticks", "", ticks_func);
	keyword_add("rnd", "(<seed>)", rnd_func);
	keyword_add("sqrt", "(n)", sqrt_func);
	keyword_add("sin", "(n, cycles)", sine_func);
	keyword_add("cos", "(n, cycles)", cosine_func);
	keyword_add("spc", "(n)", spc_func);
	keyword_add("fmt", "(\"fmt\", ...)", fmt_func);


	// FILE OPS  ==================================
	keyword_add("del", "\"file\"", cmd_del);
	keyword_add("hexdump", "\"file\"", cmd_hexdump);
	keyword_add("dir", "- list files", cmd_dir);
	keyword_add("copy", "\"src\" \"new\" <opt:[bytes]>", cmd_copy);
	keyword_add("cd", "\"letter\"", cmd_change_fs);


	// PROGRAM ==================================
	keyword_add("saveprg", "\"name\"", cmd_saveprg);
	keyword_add("loadprg", "\"name\"", cmd_loadprg);
	keyword_add("return", "- return", cmd_program_return);
	keyword_add("gosub", "[line] - call", cmd_program_gosub);
	keyword_add("goto", "[line] - jump", cmd_program_goto);
	keyword_add("end", "- end program", cmd_program_end);
	keyword_add("run", "- run program", cmd_program_run);
	keyword_add("list", "- list program", cmd_program_list);
	keyword_add("[0-n]", "\"cmdline\" - enter command line", NULL);
	keyword_add("new", "- clear program", cmd_program_new);


	// BASIC COMMANDS ==================================
	keyword_add("exit", "- exit shell", cmd_exit);
	keyword_add("prompt", "(\"text\")", cmd_prompt_expr);
	keyword_add("try", "\"cmdline\" - catch error", cmd_try);
	keyword_add("if", "[expr] \"cmdline\" - execute cmdline if expr is true", parser_if);
	keyword_add("print", "[expr] \"str\"", cmd_print);
	keyword_add("mem", "- mem info", cmd_mem);
	keyword_add("ver", "- FW build", cmd_ver);
	keyword_add("clr", "- clr vars", cmd_clr);
	keyword_add("vars", "- print vars", cmd_vars);
	keyword_add("help", "- print this help", cmd_help);

	return 0;
}
