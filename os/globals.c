#include "globals.h"
#include <string.h> // memcpy
#include <stdio.h> // EOF
#include "hal_plat.h" // t_malloc
#include "resource.h"
#include "parser.h" // execute program
#include "fs_broker.h"
#include "resource.h" // rsrc_count

stdio_stack_t *iostack;

config_t config;

program_t* program;

taskscheduler_t *scheduler;

fs_broker_t *fs;


int	program_ip;
int	program_run;
int subroutine_stack[8];
int subroutine_sp;


int load_autorun_program (void) {
	int rc;
	int fd;
	fd = open_f(fs, "autoprg", FS_O_READONLY);
	if (fd < 0) {
		return 0;
	}
	rc = program_load(program, fs, fd);
	close_f(fs, fd);

	if (rc < 1) {
		rc = 0;
	}
	return rc;
}


int load_devicecfg (void) {
	config_t lcl_config;
	int rc;
	int fd;
	fd = open_f(fs, "cfg", FS_O_READONLY);
	if (fd < 0) {
		return 0;
	}

	rc = config_load(&lcl_config, fs, fd);
	close_f(fs, fd);

	if (rc < 1) {
		rc = 0;
	}

	if (rc) {
		memcpy(&config, &lcl_config, (sizeof(config_t)));
		global_cfg_override();
	}
	return rc;
}

int save_devicecfg (void) {
	int fd;
	int rc;

	fd = open_f(fs, "cfg", FS_O_CREAT | FS_O_TRUNC);
	if (fd < 0) {
		printf_f(STDERR, "conf save:open fail");
		return 0;
	}

	rc = config_save(&config, fs, fd);
	close_f(fs, fd);
	if (rc < 1) {
		rc = 0;
	}
	return rc;
}



int execute_program (program_t *program, fifo_t* in, fifo_t* out) {
	int rc = 1;
	program_ip = 0;
	program_run = 1;
	char* line;

	if (in) {
		in->readers++;
	}
	if (out) {
		out->writers++;
	}

	while (program_run) {
		if (switchbreak()) {
			program_run = 0;
			printf_f(STDERR, "Break\n");
			break;
		}

		line = program_line(program, program_ip);
		program_ip += 1;

		parser_t *lcl_parser = parser_create(program->header.fields.linelen); // line lenght is of the program's
		if (!lcl_parser) {
			program_run = 0;
			rc = 0;
			break;
		}
        strcpy(lcl_parser->cmdbuf, line);
		rc = cmd_line_parser(lcl_parser, in, out);
		if (!rc) {
			program_run = 0;
		}

		parser_destroy(lcl_parser);

		if (program_ip >= program->header.fields.nlines) {
			program_run = 0;
			printf_f(STDERR, "Done\n");
			break;
		}
	}

	if (in) {
		in->readers--;
	}
	if (out) {
		out->writers--;
	}

	return rc;
}



int cmd_fsinfo (void) {
	//int n;
	return 1;
	//fs_dump_fat(eepromfs);
	//printf_f(STDOUT, "file_entry_t %i\n", sizeof(file_entry_t));
	//printf_f(STDOUT, "device_params_t %i\n", sizeof(device_params_t));
	//leading_wspace(0, 20);
	//printf_f(STDOUT, "%i entries free\n", fs_count_empyt_direntries(eepromfs));
	//n = fs_count_empyt_blocks(eepromfs);
	//leading_wspace(0, 20);
	//printf_f(STDOUT, "%i blocks (%i Bytes) free\n", n, (n * eepromfs->device->blocksize));
	return 1;
}


int nullfile_read (struct generic_file_s *file, int count, char *buf) {return 0;}
int nullfile_write (struct generic_file_s *file, int count, char *buf) {return count;}
int nullfile_open (struct generic_file_s *file, int flags) {return 1;}
void nullfile_close (struct generic_file_s *file) {}

int streamfile_read (struct generic_file_s* thisfile, int b, char* buf) {
	fifo_t *fifo = (fifo_t *)(thisfile->context);
	char c;
	int idx = 0;
	while (b) {
		fifo_pop_or_sleep(fifo, &c);
		buf[idx] = c;
		idx++;
		b--;
	}
	return idx;
}


int streamfile_write (struct generic_file_s* thisfile, int b, char* buf) {
	fifo_t *fifo = (fifo_t *)(thisfile->context);
	char c;
	int idx = 0;
	while (b) {
		c = buf[idx];
		fifo_push_or_sleep(fifo, &c);
		idx++;
		b--;
	}
	return idx;
}


void command_line_loop () {
	int run = 1;

	int linelen = program->header.fields.linelen;

	while (run) {
		int rc;
		int bytes;

		int prompt_pidx = 0;
		char prompt[8];
		sprintf(&(prompt[prompt_pidx]), "%c:> ", get_current_fs(fs));
		// Interpreting and executing commands, till the eternity
		printf_f(STDERR, prompt);

		int chunks = t_chunks();
		int res = rsrc_count();

		// Online command parser
		parser_t* online_parser = parser_create(linelen); // align to the program line length
		if (!online_parser) {
			printf_f(STDERR, "parser init error");
		    cpu_halt();
		}
	    char *buf = online_parser->cmdbuf;

	    bytes = read_f_line(fs, STDIN, buf, linelen-1);

        if (bytes < 1) {
			run = 0;
			break;
		}
        if (buf[bytes-1] != '\n') {
            printf_f(STDERR, "line too long\n");
            run = 0;
            break;
        }
        buf[bytes-1] = '\0';

		rc = cmd_line_parser(online_parser, NULL, NULL);
		parser_destroy(online_parser);

		chunks -= t_chunks();
        res -= rsrc_count();
        chunks -= res;
		if (chunks) { // simple memory leak check, normally shouldn't ever happen
			printf_f(STDERR, "t_malloc-t_free imbalance :%i %i\n", chunks, res);
		}
		run = (rc >= 0);
	}
}


int printf_f (int fd, const char* fmt, ...) {
	va_list ap;
	int len;
	char buf[128];

	va_start(ap, fmt);
	len = vsprintf(buf, fmt, ap);
	va_end(ap);

	write_f_all(fs, fd, buf, len);
	return len;
}
