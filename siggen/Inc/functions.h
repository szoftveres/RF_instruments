#ifndef INC_FUNCTIONS_H_
#define INC_FUNCTIONS_H_


#include <stdarg.h>
#include <stdint.h>


void ledon (void);
void ledoff (void);

int switchstate (void);

void console_putchar (const unsigned char c);
void console_printf (const char* fmt, ...);
void console_printf_e (const char* fmt, ...);

uint32_t u32_swap_endian (uint32_t n);

#endif /* INC_FUNCTIONS_H_ */
