#include "terminal_input.h"
#include <stddef.h> //NULL
#include "hal_plat.h" // malloc free


terminal_input_t* terminal_input_create (char (*getchar) (void)) {
	terminal_input_t* instance;

	instance = (terminal_input_t*)t_malloc(sizeof(terminal_input_t));
	if (!instance) {
		return instance;
	}

	instance->getchar = getchar;

	return instance;
}


void terminal_input_destroy (terminal_input_t *instance) {
	if (!instance) {
		return;
	}
	t_free(instance);
}


char* terminal_get_line (line_reader_t* reader) {
	int n = 0;
	int run = 1;

	reader->line[n] = '\0';

	while (run) {
		int printable = 1;
		char c = ((terminal_input_t *)(reader->context))->getchar();

		if (c == '\r') {
			c = '\n';
		}
		switch (c) {
		  case '\n':
			run = 0;
			break;

		  case '\b': // backspace
			if (n > 0) {
				if (printable) {
					console_printf_e("\b "); // delete
				}
				n -= 1;
				reader->line[n] = '\0';
			} else {
				printable = 0;
			}
			break;

		  default:
			if (c < 0x20 || c > 0x7E) {
				printable = 0;
				break;
			}
			reader->line[n++] = c;
			if ((n + 1) >= (reader->linelen - 1)) {
				n -= 1;
				printable = 0;
			}
			reader->line[n] = '\0';
			break;
		}
		if (printable) {
			console_printf_e("%c", c);  // echo
		}
	}
    return reader->line;
}

