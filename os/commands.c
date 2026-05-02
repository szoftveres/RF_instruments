#include "globals.h"
#include "instances.h"  // print cofg def
#include "hal_plat.h"  // t_malloc
#include "parser.h"  // lcl parsers
#include "resource.h"
#include "keyword.h"
#include <stddef.h> //NULL
#include <string.h> //strcpy

#include "pcmstream.h"


static const char* invalid_val = "Invalid value \'%i\'\n";
static const char* not_a_number = "Not a number\n";
static const char* not_a_string = "Not a string\n";
static const char* name_expected = "\"name\" expected\n";
static const char* malloc_fail = "out of memory\n";


/* Builtin Functions */

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

// Recursive "parser within parser"
int parse_str_cmd (cmd_context_s* ctxt, char* cmdstr, fifo_t* in, fifo_t* out) {
	int rc = 1;
	parser_t *lcl_parser = parser_create(strlen(cmdstr) + 2); // XXX
	if (!lcl_parser) {
		printf_f(STDERR, malloc_fail);
		return 0;
	}

	rc = cmd_line_parser(lcl_parser, cmdstr, in, out);

	parser_destroy(lcl_parser);
	return rc;
}


int parser_if (cmd_context_s* ctxt) {
	int n;

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		return 0;
	}
	n = ctxt->params->n;
	obj_consume(&(ctxt->params));

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_STR) {
		printf_f(STDERR, not_a_string);
		return 0;
	}
	if (!n) {
		obj_consume(&(ctxt->params));
		return 1; // condition is false
	}
	if (!parse_str_cmd(ctxt, ctxt->params->str, ctxt->in, ctxt->out)) {
		return 0;
	}
	obj_consume(&(ctxt->params));
	return 1;
}


int cmd_show_cfg (cmd_context_s* ctxt) {
	print_cfg();
	return 1;
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


int cmd_savecfg (cmd_context_s* ctxt) {
	int rc = save_devicecfg();
	if (rc) {
		printf_f(STDERR, "%i bytes\n", rc);
	}
	printf_f(STDERR, "cfg save %s\n", rc ? "success" : "error");

	return rc;
}


int cmd_loadcfg (cmd_context_s* ctxt) {
	int rc = load_devicecfg();
	if (rc) {
		printf_f(STDERR, "%i bytes\n", rc);
	}
	printf_f(STDERR, "cfg load %s\n", rc ? "success" : "error");

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
	subroutine_stack[subroutine_sp] = program_ip; // At this point the executing function has already increased the line number
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


int cmd_wavfilesnk (cmd_context_s* ctxt) {
	int fd;

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

	return wavsink_setup(ctxt->in, fd);
}


int cmd_nullsink (cmd_context_s* ctxt) {
	return nullsink_setup(ctxt->in);
}

int cmd_decfir (cmd_context_s* ctxt) {
	int dec;
	int bf = 1;

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		printf_f(STDERR, "dec needed\n");
		return 0;
	}
	dec = ctxt->params->n;
	obj_consume(&(ctxt->params));

	if ((dec < 2) || (dec > 64)) {
		printf_f(STDERR, "dec out of range %i\n", dec);
		return 0;
	}
	if (get_data_obj_type(ctxt->params) == OBJ_TYPE_NUM) {
		bf = ctxt->params->n;
		obj_consume(&(ctxt->params));
	}

	if ((bf < 1) || ((bf * dec) > 1024)) {
		printf_f(STDERR, "bf out of range %i\n", bf);
		return 0;
	}
	return decfir_setup(ctxt->in, ctxt->out, dec, bf);
}



int cmd_txmsg (cmd_context_s* ctxt) {
	int fs;
	int fc;
    int rc;
    char* msg;
	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		printf_f(STDERR, "fs needed\n");
		return 0;
	}
	fs = ctxt->params->n;
	obj_consume(&(ctxt->params));
	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		printf_f(STDERR, "fc needed\n");
		return 0;
	}
	fc = ctxt->params->n;
	obj_consume(&(ctxt->params));

    if (get_data_obj_type(ctxt->params) != OBJ_TYPE_STR) {
    	printf_f(STDERR, "msg needed\n");
		return 0;
    }
    msg = t_strdup(ctxt->params->str);
    obj_consume(&(ctxt->params));

	rc = txmodem_setup(ctxt->out, fs, fc, msg);
    t_free(msg);
    return rc;
}

int cmd_rxmsg (cmd_context_s* ctxt) {
	int fc;
	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		printf_f(STDERR, "fc needed\n");
		return 0;
	}
	fc = ctxt->params->n;
	obj_consume(&(ctxt->params));
	return rxmodem_setup(ctxt->in, fc);
}


/*-----------------------------------------*/

typedef struct noise_pcm_src_s {
	int samples;
} noise_pcm_src_t;


task_rc_t noise_sample_producer (void* c, uint16_t* sample_out) {
	noise_pcm_src_t *lc = (noise_pcm_src_t*)c;
	if (!(lc->samples)) {
		return TASK_RC_END;
	}
	lc->samples -= 1;
	*sample_out = (uint16_t)(rand() % 65536);
	return TASK_RC_AGAIN;
}


int cmd_noise (cmd_context_s* ctxt) {
	int fs;
	int samples;
	noise_pcm_src_t* c;

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		printf_f(STDERR, "fs needed\n");
		return 0;
	}
	fs = ctxt->params->n;
	obj_consume(&(ctxt->params));

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		printf_f(STDERR, "samples needed\n");
		return 0;
	}
	samples = ctxt->params->n;
	obj_consume(&(ctxt->params));

	c = (noise_pcm_src_t*)t_malloc(sizeof(noise_pcm_src_t));
	if (!c) {
		return 0;
	}
	c->samples = samples;
	return pcmsrc_setup(ctxt->out, fs, noise_sample_producer, NULL, c);
}

/*-----------------------------------------*/

typedef struct sine_pcm_src_s {
	int samples;
	dds_t *dds;
} sine_pcm_src_t;


task_rc_t sine_sample_producer (void* c, uint16_t* sample_out) {
	sine_pcm_src_t *lc = (sine_pcm_src_t*)c;
	int i;
	int q;
	if (!(lc->samples)) {
		return TASK_RC_END;
	}
	lc->samples -= 1;
	dds_next_sample(lc->dds, &i, &q);
	i *= (32768 / magnitude_const());
	i += 32768;
	*sample_out = (uint16_t)i;
	return TASK_RC_AGAIN;
}

void sine_producer_cleanup (void* c)  {
	sine_pcm_src_t *lc = (sine_pcm_src_t*)c;
	dds_destroy(lc->dds);
}

int cmd_sine (cmd_context_s* ctxt) {
	int fs;
	int fc;
	int samples;
	sine_pcm_src_t* c;

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		printf_f(STDERR, "fs needed\n");
		return 0;
	}
	fs = ctxt->params->n;
	obj_consume(&(ctxt->params));

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		printf_f(STDERR, "frequency needed\n");
		return 0;
	}
	fc = ctxt->params->n;
	obj_consume(&(ctxt->params));

	if (get_data_obj_type(ctxt->params) != OBJ_TYPE_NUM) {
		printf_f(STDERR, "samples needed\n");
		return 0;
	}
	samples = ctxt->params->n;
	obj_consume(&(ctxt->params));

	c = (sine_pcm_src_t*)t_malloc(sizeof(sine_pcm_src_t));
	if (!c) {
		return 0;
	}
	c->samples = samples;
	c->dds = dds_create(fs,fc); // Check is done in the worker
	return pcmsrc_setup(ctxt->out, fs, sine_sample_producer, sine_producer_cleanup, c);
}

/*-----------------------------------------*/

typedef struct wavsrc_context_s {
	fs_broker_t* fs;
	int fd;
	int samples;
	int channels;
	int bytespersample;
} wavsrc_context_t;


task_rc_t wav_pcm_producer (void* c, uint16_t* sample_out)  {
	uint32_t sample; // should be sufficient
	wavsrc_context_t *lc = (wavsrc_context_t*)c;

	if (!lc->samples) {
		return TASK_RC_END;
	}
	void *channel_within_sample = &sample;
	int final_sample = 0;
	int bytesperchannelsample = lc->bytespersample / lc->channels;

	if (read_f_all(lc->fs, lc->fd, (char*)&sample, lc->bytespersample) < lc->bytespersample) {
		//printf_f(ctxt->ferr, "read fail\n");
		return TASK_RC_END;
	}
	lc->samples -=1;

	for (int ct = 0; ct != lc->channels; ct++) {
		switch (bytesperchannelsample) {
		case 1:
			final_sample += (int)*((int8_t*)channel_within_sample);
			channel_within_sample += sizeof(int8_t);
			break;
		case 2:
			final_sample += (int)*((int16_t*)channel_within_sample);
			channel_within_sample += sizeof(int16_t);
			break;
		default:
			//printf_f(ctxt->ferr, "invalid bpcs %i\n", bytesperchannelsample);
			return TASK_RC_END;
		}
	}

	switch (bytesperchannelsample) {
	case 1:
		final_sample *= 256; // 8 bit >> 16 bit
		break;
	}
	final_sample /= lc->channels;

	*sample_out = (uint16_t)(final_sample + 32768); // signed -> unsigned
	return TASK_RC_AGAIN;
}


void wav_pcm_producer_cleanup (void* c)  {
	wavsrc_context_t *lc = (wavsrc_context_t*)c;
	close_f(lc->fs, lc->fd);
}


int cmd_wavfilesrc (cmd_context_s* ctxt) {
	int fd;
	int samplerate;

	if (!ctxt->out) {
		return 0;
	}

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

	wavsrc_context_t* c = (wavsrc_context_t*)t_malloc(sizeof(wavsrc_context_t));
	if (!c) {
		close_f(fs, fd);
		return 0;
	}
	c->fs = fs;
	c->fd = fd;

	wav_read_header(c->fs, c->fd, &samplerate, &(c->channels), &(c->bytespersample), &(c->samples));
	int time = c->samples / samplerate;

	printf_f(STDERR, "fs:%i, ch:%i, bits:%i, samples:%i, length: %i:%02i\n",
			samplerate, c->channels, 8 * (c->bytespersample / c->channels), c->samples, time / 60, time % 60);

	if (c->bytespersample > (int)sizeof(uint32_t)) {
		printf_f(STDERR, "unsupported bps:%i\n", c->bytespersample);
		wav_pcm_producer_cleanup(c);
		t_free(c);
		return 0;
	}

	return pcmsrc_setup(ctxt->out, samplerate, wav_pcm_producer, wav_pcm_producer_cleanup, c);
}


int cmd_exit (cmd_context_s* ctxt) {
	return -1;
}



/* ================================================================== */


int setup_commands (void) {


	//keyword_add("unalias", " - name", cmd_unalias);
	//keyword_add("alias", " - name \"commands\"", cmd_alias);



	// BUILTIN FUNCTIONS =======================
	keyword_add("sin", "(n cycles)", sine_func);
	keyword_add("cos", "(n cycles)", cosine_func);
	keyword_add("spc", "(n)", spc_func);
	keyword_add("fmt", "(\"fmt\" ...)", fmt_func);


	// CONFIG ==================================
	keyword_add("cfg", "- print cfg", cmd_show_cfg);
	keyword_add("savecfg", "- save config", cmd_savecfg);
	keyword_add("loadcfg", "- load config", cmd_loadcfg);


	// DSP chain ===============================
	keyword_add("rxmsg", "->rxmsg [fc]", cmd_rxmsg);
	keyword_add("txmsg", "[fs] [fc]->", cmd_txmsg);

	keyword_add("noise", "[fs] [samples]->", cmd_noise);
	keyword_add("sine", "[fs] [freq] [samples]->", cmd_sine);
	keyword_add("nullsnk", "->NULL", cmd_nullsink);
	keyword_add("df", "->decimating filter [n] <[bf]>->", cmd_decfir);
	keyword_add("wavfilesnk", "\"file\" ->WAV", cmd_wavfilesnk);
	keyword_add("wavfilesrc", "\"file\" WAV->", cmd_wavfilesrc);


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
	keyword_add("if", "[expr] \"cmdline\" - execute cmdline if expr is true", parser_if);
	keyword_add("print", "[expr] \"str\"", cmd_print);
	keyword_add("mem", "- mem info", cmd_mem);
	keyword_add("ver", "- FW build", cmd_ver);
	keyword_add("clr", "- clr vars", cmd_clr);
	keyword_add("vars", "- print vars", cmd_vars);
	keyword_add("help", "- print this help", cmd_help);

	return 0;
}
