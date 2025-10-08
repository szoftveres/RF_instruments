
#include "functions.h"
#include <string.h> // strlen
#include <stdio.h> // vsprintf
#include <stdlib.h> // malloc free
#include "main.h"


extern UART_HandleTypeDef huart1;

void ledon (void) {
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
}

void ledoff (void) {
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
}

int switchstate (void) {
	return HAL_GPIO_ReadPin(SWITCH_GPIO_Port, SWITCH_Pin);
}

void console_putchar (const unsigned char c) {
	HAL_UART_Transmit(&huart1, &c, 1, 1);
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
		HAL_UART_Transmit(&huart1, (uint8_t*)buf, len, len);
	}
	HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", 2, 1);
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
		HAL_UART_Transmit(&huart1, (uint8_t*)buf, len, len);
	}
	return len;
}


uint32_t u32_swap_endian (uint32_t n) {
	return (((n & 0xFF) << 24) +
			((n & 0xFF00) << 8) +
		    ((n & 0xFF0000) >> 8) +
		    ((n & 0xFF000000) >> 24));
}


/* ================================ */


fifo_t* fifo_create (int length, int wordlength) {
	fifo_t *instance;

	instance = (fifo_t*)malloc(sizeof(fifo_t));
	if (!instance) {
		return instance;
	}
	instance->buf = (char*)malloc(length * wordlength);
	if (!instance->buf) {
		free(instance);
		instance = NULL;
		return instance;
	}
	instance->length = length;
	instance->wordlength = wordlength;
	instance->ip = 0;
	instance->op = 0;
	instance->data = 0;
	return instance;
}


void fifo_destroy (fifo_t *instance) {
	if (!instance) {
		return;
	}
	if (instance->buf) {
		free(instance->buf);
	}
	free(instance);
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


