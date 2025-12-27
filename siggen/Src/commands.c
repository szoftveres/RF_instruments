#include "instances.h"
#include "hal_plat.h"  // t_malloc
#include "parser.h"  // lcl parsers
#include "resource.h"
#include "keyword.h"
#include <stddef.h> //NULL
#include <string.h> //strcpy

#include "pcmstream.h"



#include "stm32f4xx_hal.h" // xXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX


static const char* invalid_val = "Invalid value \'%i\'";
static const char* not_a_string = "Not a string";
static const char* name_expected = "\"name\" expected";
static const char* malloc_fail = "out of memory";


// Recursive "parser within parser"
int parse_str_cmd (char* cmdstr, fifo_t* in, fifo_t* out) {
	int rc = 1;
	parser_t *lcl_parser = parser_create(strlen(cmdstr) + 2); // XXX
	if (!lcl_parser) {
		console_printf(malloc_fail);
		return 0;
	}

	rc = cmd_line_parser(lcl_parser, cmdstr, in, out);

	parser_destroy(lcl_parser);
	return rc;
}


int parser_if (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	int n;

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_NUM) {
		return 0;
	}
	n = (*params)->n;
	cmd_param_consume(params);

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_STR) {
		console_printf(not_a_string);
		return 0;
	}
	if (!n) {
		cmd_param_consume(params);
		return 1; // condition is false
	}
	if (!parse_str_cmd((*params)->str, in, out)) {
		return 0;
	}
	cmd_param_consume(params);
	return 1;
}


int cmd_show_cfg (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	print_cfg();
	return 1;
}


int cmd_sleep (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	int ms;

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_NUM) {
		return 0;
	}
	ms = (*params)->n;
	cmd_param_consume(params);

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


int cmd_print (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	int res;
	int rc = 0;

	do {
		res = 0;

		if (get_cmd_arg_type(params) == CMD_ARG_TYPE_NUM) {
			console_printf_e("%i", (*params)->n);
			res = 1;
			cmd_param_consume(params);
		}

		if (get_cmd_arg_type(params) == CMD_ARG_TYPE_STR) {
			console_printf_e("%s", (*params)->str);
			res = 1;
			cmd_param_consume(params);
		}
		rc |= res;
	} while (res);

	if (rc) {
		console_printf("");
	}
	return rc;
}


int cmd_savecfg (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	int rc = save_devicecfg();
	if (rc) {
		console_printf("%i bytes", rc);
	}
	console_printf("cfg save %s", rc ? "success" : "error");

	return rc;
}


int cmd_loadcfg (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	int rc = load_devicecfg();
	if (rc) {
		console_printf("%i bytes", rc);
	}
	console_printf("cfg load %s", rc ? "success" : "error");

	return rc;
}


int cmd_saveprg (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	int fd;
	int rc;

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_STR) {
		console_printf(name_expected);
		return 0;
	}

	fd = open_f(fs, (*params)->str, FS_O_CREAT | FS_O_TRUNC);
	cmd_param_consume(params);

	if (fd < 0) {
		console_printf("open fail");
		return 0;
	}
	rc = program_save(program, fs, fd);

	close_f(fs, fd);
	if (rc > 0) {
		console_printf("%i bytes", rc);
	}
	console_printf("prg save %s", rc > 0 ? "success" : "error");

	return rc;
}


int cmd_loadprg (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	int fd;
	int rc;

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_STR) {
		console_printf(name_expected);
		return 0;
	}

	fd = open_f(fs, (*params)->str, FS_O_READONLY);
	cmd_param_consume(params);
	if (fd < 0) {
		console_printf("open fail");
		return 0;
	}
	rc = program_load(program, fs, fd);

	close_f(fs, fd);
	if (rc > 0) {
		console_printf("%i bytes", rc);
	}
	console_printf("prg load %s", rc > 0 ? "success" : "error");

	return rc;
}


int cmd_program_new (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	char* endstr = "";
	char* line;

	for (int i = 0; i != program->header.fields.nlines; i++) {
		line = program_line(program, i);
		strcpy(line, endstr);
	}
	return 1;
}


int cmd_program_list (cmd_param_t** params, fifo_t* in, fifo_t* out) {
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


int cmd_program_end (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	program_run = 0;
	console_printf("Done");
	return 1;
}


int cmd_program_run (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	execute_program(program, in, out);
	return 1;
}


int cmd_program_goto (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	int line;
	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_NUM) {
		return 0;
	}
	line = (*params)->n;
	cmd_param_consume(params);
	if (line < 0 || line >= program->header.fields.nlines) {
		console_printf(invalid_val, line);
		return 0;
	}
	program_ip = line;
	return 1;
}


int cmd_program_gosub (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	int line;
	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_NUM) {
		return 0;
	}
	line = (*params)->n;
	cmd_param_consume(params);
	if (line < 0 || line >= program->header.fields.nlines) {
		console_printf(invalid_val, line);
		return 0;
	}
	subroutine_stack[subroutine_sp] = program_ip; // At this point the executing function has already increased the line number
	subroutine_sp += 1; // TODO error handling
	program_ip = line;
	return 1;
}


int cmd_program_return (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	if (!subroutine_sp) {
		console_printf("Not in a subroutine");
		return 0;
	}
	subroutine_sp -= 1;
	program_ip = subroutine_stack[subroutine_sp];
	return 1;
}


int cmd_vars (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	for (resource_t* r = resource_it_start(); r; r = resource_it_next(r)) {
		console_printf("%s : %i", r->name, r->get(r));
	}
	return 1;
}


int cmd_ver (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	console_printf("%s - %s", __DATE__ ,__TIME__);
	return 1;
}


int cmd_mem (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	console_printf("chunks:%i", t_chunks());
	return 1;
}


int cmd_fsinfo (void) {
	//int n;
	return 1;
	//fs_dump_fat(eepromfs);
	//console_printf("file_entry_t %i", sizeof(file_entry_t));
	//console_printf("device_params_t %i", sizeof(device_params_t));
	leading_wspace(0, 20);
	//console_printf("%i entries free", fs_count_empyt_direntries(eepromfs));
	//n = fs_count_empyt_blocks(eepromfs);
	leading_wspace(0, 20);
	//console_printf("%i blocks (%i Bytes) free", n, (n * eepromfs->device->blocksize));
	return 1;
}

#include "fatsmall_fs.h"
int cmd_format (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	char* line = terminal_get_line(online_input, " type \"yes\"> ", 1);
	if (strcmp(line, "yes")) {
		console_printf("aborted");
		return 1;
	}
	fs_format(eepromfs, 16);
	return cmd_fsinfo();
}


int cmd_del (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	int rc;

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_STR) {
		console_printf(name_expected);
		return 0;
	}
	rc = delete_f(fs, (*params)->str);
	cmd_param_consume(params);
	if (rc < 0) {
		console_printf("delete fail");
	}

	return 1;
}


int
cmd_change_fs (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	int rc;

    if (get_cmd_arg_type(params) != CMD_ARG_TYPE_STR) {
		console_printf(name_expected);
		return 0;
	}
    rc = change_current_fs(fs, (*params)->str[0]);
	cmd_param_consume(params);
	if (!rc) {
		console_printf("Invalid fs");
	}
	return rc;
}



int cmd_dir (cmd_param_t** params, fifo_t* in, fifo_t* out) {

	int rc;
	char* name;
	int size;

	opendir_f(fs);
	while ((rc = walkdir_f(fs, &name, &size))) {
		int nlen;
		nlen = strlen(name);
		console_printf_e("%s", name);
		leading_wspace(nlen, 20);
		nlen = console_printf_e("%i", size);
		leading_wspace(nlen, 20);
		//console_printf("attr:0x%04x,start:0x%04x", entry->attrib, entry->start);
		console_printf("");
	}
	closedir_f(fs);
	return cmd_fsinfo();
}


int
cmd_hexdump (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	char buf[16];
	int fd;
	int rc = 16;
    int i;
    int addr = 0;

    if (get_cmd_arg_type(params) != CMD_ARG_TYPE_STR) {
		console_printf(name_expected);
		return 0;
	}

	fd = open_f(fs, (*params)->str, FS_O_READONLY);
	cmd_param_consume(params);
	if (fd < 0) {
		console_printf("open fail");
		return 0;
	}

    while (rc == 16) {
    	rc = read_f(fs, fd, buf, 16);
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

	close_f(fs, fd);

    return 1;
}

#define COPY_UNIT (32)   // XXX for now
int cmd_copy (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	int fdsrc;
	int fdnew;
	int totalbytes = 0;
	void* buf;

	buf = t_malloc(COPY_UNIT);
	if (!buf) {
	    console_printf(malloc_fail);
	    return 0;
	}

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_STR) {
		console_printf(name_expected);
		t_free(buf);
		return 0;
	}

	fdsrc = open_f(fs, (*params)->str, FS_O_READONLY);
	cmd_param_consume(params);
	if (fdsrc < 0) {
		console_printf("src open fail");
		t_free(buf);
		return 0;
	}

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_STR) {
		console_printf(name_expected);
		close_f(fs, fdsrc);
		t_free(buf);
		return 0;
	}

	fdnew = open_f(fs, (*params)->str, FS_O_CREAT | FS_O_TRUNC);
	cmd_param_consume(params);

	if (fdnew < 0) {
		console_printf("dst open fail");
		close_f(fs, fdsrc);
		t_free(buf);
		return 0;
	}

	int b;
	while ((b = read_f(fs, fdsrc, buf, COPY_UNIT)) > 0) {
		if (write_f(fs, fdnew, buf, b) != b) {
			console_printf("disk full");
			break;
		}
		totalbytes += b;
	}

	close_f(fs, fdnew);
	close_f(fs, fdsrc);
	t_free(buf);

	console_printf("%i bytes copied", totalbytes);

	return 1;
}


/*
int cmd_fat (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	fs_dump_fat(eepromfs);
	return 1;
}
*/


int cmd_rfon (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	set_rf_output(1);
	print_cfg();
	return 1;
}


int cmd_rfoff (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	set_rf_output(0);
	print_cfg();
	return 1;
}


int cmd_amtone (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	int ms;

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_NUM) {
		return 0;
	}
	ms = (*params)->n;
	cmd_param_consume(params);

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


int cmd_fmtone (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	int ms;
	int dev;

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_NUM) {
		return 0;
	}
	dev = (*params)->n;
	cmd_param_consume(params);

	if (dev < 10 || dev > 1000) { // 10 kHz - 1 MHz
		console_printf(invalid_val, dev);
		return 0;
	}

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_NUM) {
		return 0;
	}
	ms = (*params)->n;
	cmd_param_consume(params);

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


int cmd_help (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	keyword_t *kw = keyword_it_start();
	while (kw) {
		console_printf("  %s %s", kw->token, kw->helpstr);
		kw = keyword_it_next(kw);
	}
	return 1;
}


int cmd_alias (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	char *aliasname;
	char *aliascmd;
	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_STR) {
		console_printf(not_a_string);
		return 0;
	}
	aliasname = t_strdup((*params)->str);
	cmd_param_consume(params);

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_STR) {
		console_printf(not_a_string);
		t_free(aliasname);
		return 0;
	}
	aliascmd = t_strdup((*params)->str);
	cmd_param_consume(params);

	keyword_add(aliasname, aliascmd, NULL);
	return 1;
}


int cmd_unalias (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	keyword_t *kw;
	int rc = 0;

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_STR) {
		console_printf(not_a_string);
		return 0;
	}
	/* TODO check if it was an alias (i.e. not a regular cmd) */
	kw = keyword_remove((*params)->str);
	cmd_param_consume(params);

	if (kw) {
		t_free(kw->token);
		t_free(kw->helpstr);
		rc = 1;
	}

	return rc;
}


int cmd_wavfilesnk (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	int fd;

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_STR) {
		console_printf(name_expected);
		return 0;
	}

	fd = open_f(fs, (*params)->str, FS_O_CREAT | FS_O_TRUNC);
	cmd_param_consume(params);
	if (fd < 0) {
		console_printf("open fail");
		return 0;
	}

	return wavsink_setup(in, fd);
}


int cmd_nullsink (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	return nullsink_setup(in);
}

int cmd_decfir (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	int dec;
	int bf = 1;

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_NUM) {
		console_printf("dec needed");
		return 0;
	}
	dec = (*params)->n;
	cmd_param_consume(params);

	if ((dec < 2) || (dec > 64)) {
		console_printf("bf out of range %i", dec);
		return 0;
	}
	if (get_cmd_arg_type(params) == CMD_ARG_TYPE_NUM) {
		bf = (*params)->n;
		cmd_param_consume(params);
	}

	if ((bf < 1) || ((bf * dec) > 1024)) {
		console_printf("bf out of range %i", bf);
		return 0;
	}
	return decfir_setup(in, out, dec, bf);
}



int cmd_txpkt (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	int fs;
	int fc;
	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_NUM) {
		console_printf("fs needed");
		return 0;
	}
	fs = (*params)->n;
	cmd_param_consume(params);
	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_NUM) {
		console_printf("fc needed");
		return 0;
	}
	fc = (*params)->n;
	cmd_param_consume(params);
	return txmodem_setup(out, fs, fc);
}

int cmd_rxpkt (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	int fc;
	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_NUM) {
		console_printf("fc needed");
		return 0;
	}
	fc = (*params)->n;
	cmd_param_consume(params);
	return rxmodem_setup(in, fc);
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
	*sample_out = (uint16_t)(rnd_getter(NULL) % 65536);
	return TASK_RC_AGAIN;
}


int cmd_noise (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	int fs;
	int samples;
	noise_pcm_src_t* c;

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_NUM) {
		console_printf("fs needed");
		return 0;
	}
	fs = (*params)->n;
	cmd_param_consume(params);

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_NUM) {
		console_printf("samples needed");
		return 0;
	}
	samples = (*params)->n;
	cmd_param_consume(params);

	c = (noise_pcm_src_t*)t_malloc(sizeof(noise_pcm_src_t));
	if (!c) {
		return 0;
	}
	c->samples = samples;
	return pcmsrc_setup(out, fs, noise_sample_producer, NULL, c);
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

int cmd_sine (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	int fs;
	int fc;
	int samples;
	sine_pcm_src_t* c;

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_NUM) {
		console_printf("fs needed");
		return 0;
	}
	fs = (*params)->n;
	cmd_param_consume(params);

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_NUM) {
		console_printf("frequency needed");
		return 0;
	}
	fc = (*params)->n;
	cmd_param_consume(params);

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_NUM) {
		console_printf("samples needed");
		return 0;
	}
	samples = (*params)->n;
	cmd_param_consume(params);

	c = (sine_pcm_src_t*)t_malloc(sizeof(sine_pcm_src_t));
	if (!c) {
		return 0;
	}
	c->samples = samples;
	c->dds = dds_create(fs,fc); // Check is done in the worker
	return pcmsrc_setup(out, fs, sine_sample_producer, sine_producer_cleanup, c);
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

	if (read_f(lc->fs, lc->fd, (char*)&sample, lc->bytespersample) < lc->bytespersample) {
		console_printf("read fail");
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
			console_printf("invalid bpcs %i", bytesperchannelsample);
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


int cmd_wavfilesrc (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	int fd;
	int samplerate;

	if (!out) {
		return 0;
	}

	if (get_cmd_arg_type(params) != CMD_ARG_TYPE_STR) {
		console_printf(name_expected);
		return 0;
	}

	fd = open_f(fs, (*params)->str, FS_O_READONLY);
	cmd_param_consume(params);
	if (fd < 0) {
		console_printf("open fail");
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

	console_printf("fs:%i, ch:%i, bits:%i, samples:%i, length: %i:%02i",
			samplerate, c->channels, 8 * (c->bytespersample / c->channels), c->samples, time / 60, time % 60);

	if (c->bytespersample > sizeof(uint32_t)) {
		console_printf("unsupported bps:%i", c->bytespersample);
		wav_pcm_producer_cleanup(c);
		t_free(c);
		return 0;
	}

	return pcmsrc_setup(out, samplerate, wav_pcm_producer, wav_pcm_producer_cleanup, c);
}

/*-----------------------------------------*/

#include <stdlib.h>

int cmd_malloctest (cmd_param_t** params, fifo_t* in, fifo_t* out) {
	int i = 0;
	const int size = 1024;
	while (malloc(size)) {
		i++;
	}
	console_printf("size:%i, total:%i", size, size*i);
	cpu_halt();
	return 1;
}

/* ================================================================== */


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


	keyword_add("malloctest", "", cmd_malloctest);


	// DSP chain ===============================
	keyword_add("rxpkt", "->rxpkt [fc]", cmd_rxpkt);
	keyword_add("txpkt", "[fs] [fc]->", cmd_txpkt);

	keyword_add("noise", "noise [fs] [samples]->", cmd_noise);
	keyword_add("sine", "sine [fs] [freq] [samples]->", cmd_sine);
	keyword_add("nullsnk", "->NULL", cmd_nullsink);
	keyword_add("df", "->decimating filter [n] <[bf]>->", cmd_decfir);
	keyword_add("wavfilesnk", "\"file\" ->WAV", cmd_wavfilesnk);
	keyword_add("wavfilesrc", "\"file\" WAV->", cmd_wavfilesrc);


	// FILE OPS  ==================================
	keyword_add("format", "- format EEPROM", cmd_format);
	keyword_add("del", "\"file\" - del file", cmd_del);
	keyword_add("hexdump", "\"file\"", cmd_hexdump);
	keyword_add("dir", "- list files", cmd_dir);
	keyword_add("copy", "\"src\" \"new\"", cmd_copy);
	keyword_add("cd", "\"letter\"", cmd_change_fs);


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
	keyword_add("mem", "- mem info", cmd_mem);
	keyword_add("ver", "- FW build", cmd_ver);
	keyword_add("vars", "- print rsrc vars", cmd_vars);
	keyword_add("help", "- print this help", cmd_help);

	return 0;
}
