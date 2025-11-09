#ifndef INC_FUNCTIONS_H_
#define INC_FUNCTIONS_H_


#include <stdarg.h>
#include <stdint.h>


void ledon (void);
void ledoff (void);

int switchstate (void);

void console_putchar (const unsigned char c);
void console_printf (const char* fmt, ...);
int console_printf_e (const char* fmt, ...);
void leading_wspace (int start, int stop);

uint32_t u32_swap_endian (uint32_t n);


typedef struct fifo_s {
	char *buf;
	int length;
	int wordlength;
	int ip;
	int op;
	int data;
} fifo_t;

fifo_t* fifo_create (int length, int wordlength);
void fifo_destroy (fifo_t *instance);
int fifo_push (fifo_t* instance, void *c);
int fifo_pop (fifo_t* instance, void *c);



#endif /* INC_FUNCTIONS_H_ */
