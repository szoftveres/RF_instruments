#include "line_reader.h"
#include <stddef.h> //NULL
#include "hal_plat.h" // t_malloc


line_reader_t* line_reader_create (int linelen, void *context) {

	line_reader_t* instance = (line_reader_t*)t_malloc(sizeof(line_reader_t));
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
	instance->context = context;

	return instance;
}


void line_reader_destroy (line_reader_t *instance) {
	if (!instance) {
		return;
	}
	if (instance->line) {
		t_free(instance->line);
	}
	t_free(instance);
}
