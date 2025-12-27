#include "fifo.h"
#include "hal_plat.h" // t_malloc, cpu_sleep
#include <string.h> // memcpy



fifo_t* fifo_create (int length, int wordlength) {
	fifo_t *instance;

	instance = (fifo_t*)t_malloc(sizeof(fifo_t));
	if (!instance) {
		return instance;
	}
	instance->buf = (char*)t_malloc(length * wordlength);
	if (!instance->buf) {
		t_free(instance);
		instance = NULL;
		return instance;
	}
	instance->length = length;
	instance->wordlength = wordlength;
	instance->ip = 0;
	instance->op = 0;
	instance->data = 0;
	instance->writers = 0;
	instance->readers = 0;
	return instance;
}


void fifo_destroy (fifo_t *instance) {
	if (!instance) {
		return;
	}
	if (instance->readers || instance->writers) {
		console_printf("fifo still in use");
	}
	if (instance->buf) {
		t_free(instance->buf);
	}
	t_free(instance);
}


int fifo_isempty (fifo_t *instance) {
	return ((instance->ip == instance->op));
}

int fifo_push (fifo_t* instance, void *c) {
	if (((instance->ip + 1) & (instance->length - 1)) == instance->op) {
		return 0; // full
	}
	memcpy(&(instance->buf[instance->ip * instance->wordlength]), c, instance->wordlength);
	instance->ip = (instance->ip + 1) & (instance->length - 1);
	instance->data += 1;
	return 1;
}


int fifo_pop (fifo_t* instance, void *c) {
	if (!instance->data || (instance->ip == instance->op)) {
		return 0; // empty
	}
	instance->data -= 1;
	memcpy(c, &(instance->buf[instance->op * instance->wordlength]), instance->wordlength);
 	instance->op = (instance->op + 1) & (instance->length - 1);
 	return 1;
}


void fifo_sleep (fifo_t* instance, void *c, int (*fifo_op) (fifo_t* , void *)) {
	periodic_IT_off();
	while (!fifo_op(instance, c)) {
		cpu_sleep();
	}
	periodic_IT_on();
}


void fifo_push_or_sleep (fifo_t* instance, void *c) {
	fifo_sleep(instance, c, fifo_push);
}

void fifo_pop_or_sleep (fifo_t* instance, void *c) {
	fifo_sleep(instance, c, fifo_pop);
}


