#include "terminal_input.h"
#include <stddef.h> //NULL
#include "hal_plat.h" // malloc free


terminal_input_t* terminal_input_create (char (*getchar) (void), int linelen) {
	terminal_input_t* instance;

	instance = (terminal_input_t*)t_malloc(sizeof(terminal_input_t));
	if (!instance) {
		return instance;
	}
	instance->line = (char*)t_malloc(linelen);
	if (!instance->line) {
		t_free(instance);
		instance = NULL;
		return instance;
	}
	instance->linelen = linelen;
	instance->getchar = getchar;

	return instance;
}


void terminal_input_destroy (terminal_input_t *instance) {
	if (!instance) {
		return;
	}
	if (instance->line) {
		t_free(instance->line);
	}
	t_free(instance);
}


char* terminal_get_line (terminal_input_t* instance, const char* prompt, int echo) {
	int n = 0;
	int run = 1;

	instance->line[n] = '\0';
	if (prompt) {
		console_printf_e(prompt);
	}
	while (run) {
		int printable = echo;
		char c = instance->getchar();

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
				instance->line[n] = '\0';
			} else {
				printable = 0;
			}
			break;

		  default:
			if (c < 0x20 || c > 0x7E) {
				printable = 0;
				break;
			}
			instance->line[n++] = c;
			if ((n + 1) >= (instance->linelen - 1)) {
				n -= 1;
				printable = 0;
			}
			instance->line[n] = '\0';
			break;
		}
		if (printable) {
			console_printf_e("%c", c);  // echo
		}
	}
    return instance->line;
}

