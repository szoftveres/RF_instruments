#include "os/hal_plat.h"

static int switchstate = 0;
static int ticks = 0;

int ticks_getter (void) {
	return ticks++;
}


int rnd_setter (int rand_set) {
	return 1;
}

int rnd_getter (void) {
	return 1;
}


int dac_max (void) {
	return 0;
}


void ledon (void) {
	;
}

void ledoff (void) {
	;
}


void delay_ms (uint32_t delay) {
}

void ledflash (int n) {
}

int switchbreak (void) {
}

void console_putchar (const unsigned char c) {
}

void console_printf (const char* fmt, ...) {
}


int console_printf_e (const char* fmt, ...) {
}


uint32_t u32_swap_endian (uint32_t n) {
	return (((n & 0xFF) << 24) +
			((n & 0xFF00) << 8) +
		    ((n & 0xFF0000) >> 8) +
		    ((n & 0xFF000000) >> 24));
}


static int chunks = 0;

void *t_malloc (size_t size) {
	void *p = NULL ;  //malloc(size);
	if (p) {
		chunks += 1;
	} else {
		console_printf("t_malloc fail size:%i chunks:%i", size, chunks);
		while (1) {periodic_IT_off(); cpu_sleep();}
	}
	return p;
}

char *t_strdup (const char *c) {
	char *p = NULL ; //strdup(c);
	if (p) {
		chunks += 1;
	} else {
		console_printf("t_strdup fail s:%s chunks:%i", c, chunks);
		while (1) {periodic_IT_off(); cpu_sleep();}
	}
	return p;
}

void t_free (void *ptr) {
	chunks -= 1;
	//free(ptr);
}

int t_chunks (void) {
	return chunks;
}

/* ================================ */

char* grayscale = " `.-':_,^=;><+!rc*/z?sLTv)J7(|Fi{C}fI31tlu[neoZ5Yxjya]2ESwqkP6h9d4VpOGbUAKXHm8R@";

static int grayscale_n = 0;

char level_to_color (int n, int range) {
	if (n >= range) {
		n = range -1;
	}
	if (n < 0) {
		n = 0;
	}
	if (!grayscale_n) {
		grayscale_n = strlen(grayscale);
	}
	return grayscale[n * grayscale_n / range];
}

/* ================================ */

void cpu_sleep (void) {
	;
}

void periodic_IT_off (void) {
	;
}

void periodic_IT_on (void) {
	;
}

void cpu_halt (void) {
	console_printf("System halted");
	while(1) {periodic_IT_off();cpu_sleep();}
}



