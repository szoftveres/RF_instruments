#include "os/hal_plat.h"
#include <string.h> // strlen
#include <stdio.h> // vsprintf
#include <stdlib.h> // malloc free
#include <unistd.h> // unix write


int ticks_getter (void * context) {
	return 0;
}


int rnd_setter (void * context, int rand_set) {
	srand(rand_set);
	return 1;
}

int rnd_getter (void * context) {
	return rand();
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
	;
}

void ledflash (int n) {
	while (n) {
		ledon();
		delay_ms(75);
		ledoff();
		delay_ms(75);
		n -= 1;
	}
}

int switchbreak (void) {
	return 0;
}

void console_putchar (const unsigned char c) {
    int out = (int)c;
    putchar(out);
}

void console_printf (const char* fmt, ...) {
	va_list ap;
	int len;
	char buf[128];

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);

	len = strlen(buf);
	if (len) {
        write(1, buf, len);
	}
    write(1, "\n", 1);
	//puts("\n");
}


int console_printf_e (const char* fmt, ...) {
	va_list ap;
	int len;
	char buf[128];

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);

	len = strlen(buf);
	if (len) {
        write(1, buf, len);
	}
	return len;
}


void leading_wspace (int start, int stop) {
	for (int i = start; i < stop; i++) {
		console_printf_e(" ");
	}
}

uint32_t u32_swap_endian (uint32_t n) {
	return (((n & 0xFF) << 24) +
			((n & 0xFF00) << 8) +
		    ((n & 0xFF0000) >> 8) +
		    ((n & 0xFF000000) >> 24));
}


static int chunks = 0;

void *t_malloc (size_t size) {
	void *p = malloc(size);
	if (p) {
		chunks += 1;
	} else {
		console_printf("t_malloc fail size:%i chunks:%i", size, chunks);
		while (1) {periodic_IT_off(); cpu_sleep();}
	}
	return p;
}

char *t_strdup (const char *c) {
	char *p = strdup(c);
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
	free(ptr);
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

/* ================================ */
/*


void (*sampler_callback) (void*);
void* sampler_context;

int set_sampler_frequency (int fs) {
	uint32_t TIM2Clock = 200000000;
	uint32_t ReloadValue = (TIM2Clock / (fs)) - 1;

	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 0;
	htim2.Init.CounterMode = TIM_COUNTERMODE_DOWN;
	htim2.Init.Period = ReloadValue;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
		return 0;
	}
	return 1;
}


void start_sampler (void(*cb)(void*), void* ctxt) {
	sampler_callback = cb;
	sampler_context = ctxt;
	HAL_TIM_Base_Start_IT(&htim2);
}


void stop_sampler (void) {
	HAL_TIM_Base_Stop_IT(&htim2);
}


*/

/*
int set_TIM_reload_frequency (int fs) {
	// STM32F446

    // PCLK1 prescaler equal to 1 => TIMCLK = PCLK1
    // PCLK1 prescaler different from 1 => TIMCLK = 2 * PCLK1
	uint32_t TIM2Clock = HAL_RCC_GetPCLK1Freq() * ((RCC->CFGR & RCC_CFGR_PPRE1) ? 2 : 1);
	uint32_t ReloadValue = (TIM2Clock / (fs))-1;

	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 0;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = ReloadValue;
	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
		return 0;
	}
	return 1;
}
*/


