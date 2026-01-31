#ifndef INC_HAL_PLAT_H_
#define INC_HAL_PLAT_H_

#include <stdarg.h>  // console_printf
#include <stdint.h> // uint..
#include <stddef.h> // size_t

int ticks_getter (void * context);

int rnd_setter (void * context, int rand_set);
int rnd_getter (void * context);

int pushbutton_getter (void * context);

void ledon (void);
void ledoff (void);
void delay_ms (uint32_t delay);
void ledflash (int n);

int switchbreak (void);
int pushbuttonstate (void);

void console_putchar (const unsigned char c);
void console_printf (const char* fmt, ...);
int console_printf_e (const char* fmt, ...);
void leading_wspace (int start, int stop);

uint32_t u32_swap_endian (uint32_t n);


void *t_malloc (size_t size);
char *t_strdup (const char *c);
void t_free (void *ptr);
int t_chunks (void);

char level_to_color (int n, int range);



void cpu_sleep (void);
void periodic_IT_off (void);
void periodic_IT_on (void);
void cpu_halt();


extern void (*sampler_callback) (void*);
extern void* sampler_context;

int set_sampler_frequency (int fs);

void start_sampler (void(*cb)(void*), void* ctxt);
void stop_sampler (void);


int start_audio_in (int fs);
void stop_audio_in (void);
int start_audio_out (int fs);
void stop_audio_out (void);
int record_int16_sample (int16_t *s);
int play_int16_sample (int16_t *s);
int play_silence_ms (int ms);



#endif /* INC_HAL_PLAT_H_ */
