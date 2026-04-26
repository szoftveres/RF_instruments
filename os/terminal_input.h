#ifndef INC_TERMINAL_INPUT_H_
#define INC_TERMINAL_INPUT_H_


typedef struct terminal_input_s {
	char (*getchar) (void);
	void (*putchar) (char);
    int echo;
    int pos;
    int rp;
	int linelen;
	char *line;
} terminal_input_t;


terminal_input_t* terminal_input_create (char (*getchar) (void), void (*putchar) (char), int echo, int linelen);
void terminal_input_destroy (terminal_input_t *instance);


#include "fs_broker.h"
int consolefile_readline_canonical (struct generic_file_s* context, int b, char* buf);
int consolefile_readline_raw (struct generic_file_s* thisfile, int b, char* buf);
int consolefile_read (struct generic_file_s* thisfile, int b, char* buf);
int consolefile_write (struct generic_file_s* thisfile, int count, char* buf);


#endif /* INC_TERMINAL_INPUT_H_ */
