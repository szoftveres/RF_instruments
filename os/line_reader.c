#include "line_reader.h"
#include <stddef.h> //NULL
#include "hal_plat.h" // malloc free


line_reader_t* line_reader_create (int linelen, char* (*getline) (struct line_reader_s*), void *context) {

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
	instance->getline = getline;
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
