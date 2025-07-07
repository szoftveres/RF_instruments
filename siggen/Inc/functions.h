#ifndef INC_FUNCTIONS_H_
#define INC_FUNCTIONS_H_


#include <stdarg.h>

void ledon (void);
void ledoff (void);

void console_putchar (const unsigned char c);
void console_printf (const char* fmt, ...);
void console_printf_e (const char* fmt, ...);


#endif /* INC_FUNCTIONS_H_ */
