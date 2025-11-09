#include "terminal_input.h"
#include "functions.h"
#include <stddef.h> //NULL
#include <stdlib.h> // malloc free


terminal_input_t* terminal_input_create (char (*getchar) (void), int linelen) {
	terminal_input_t* instance;

	instance = (terminal_input_t*)malloc(sizeof(terminal_input_t));
	if (!instance) {
		return instance;
	}
	instance->line = (char*)malloc(linelen);
	if (!instance->line) {
		free(instance);
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
		free(instance->line);
	}
	free(instance);
}


char* terminal_get_line (terminal_input_t* instance, const char* prompt, int echo) {
	int n = 0;

	instance->line[n] = '\0';
	if (echo && prompt) {
		console_printf_e(prompt);
	}
	while (1) {
		char c = instance->getchar();

		if (c == '\r') {
			c = '\n';
		}
		if (echo) {
			console_printf_e("%c", c);  // echo
		}
		switch (c) {
		case '\n':
			return instance->line;

		case '\b': // backspace
			if (n > 0) {
				if (echo) {
					console_printf_e(" \b"); // delete
				}
				n -= 1;
				instance->line[n] = '\0';
			}
			break;

		default:
			instance->line[n++] = c;
			if ((n + 1) >= (instance->linelen - 1)) {
				n -= 1;
				if (echo) {
					console_printf_e("\b");
				}
			}
			instance->line[n] = '\0';
			break;
		}
	}
    return instance->line;
}

