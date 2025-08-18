
#include "functions.h"
#include <string.h> // strlen
#include <stdio.h> // vsprintf
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

	len = strlen(buf)+1;
	HAL_UART_Transmit(&huart1, (uint8_t*)buf, len, len);
	HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", 2, 1);
}


void console_printf_e (const char* fmt, ...) {
	va_list ap;
	int len;
	char buf[128];

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);

	len = strlen(buf)+1;
	HAL_UART_Transmit(&huart1, (uint8_t*)buf, len, len);
}


uint32_t u32_swap_endian (uint32_t n) {
	return (((n & 0xFF) << 24) +
			((n & 0xFF00) << 8) +
		    ((n & 0xFF0000) >> 8) +
		    ((n & 0xFF000000) >> 24));
}

