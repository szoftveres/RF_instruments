#include <eeprom.h>
#include "stm32f4xx_hal.h"
#include <string.h> // memcpy


extern I2C_HandleTypeDef hi2c1;

#define AT24_ADDR (0xA0)
#define EEPROM_PAGE (16)

uint8_t
eeprom_read_byte (uint16_t address) {
	uint16_t devaddres = AT24_ADDR | ((address / 256) << 1);
	uint8_t addressword = address & 0xFF;
	uint8_t data = 0x55;

	HAL_I2C_Master_Transmit(&hi2c1, devaddres, &addressword, 1, 500);
	HAL_I2C_Master_Receive(&hi2c1, AT24_ADDR, &data, 1, 500);

	return data;
}


void
eeprom_write_byte (uint16_t address, uint8_t data) {
	uint16_t devaddres = AT24_ADDR | ((address / 256) << 1);
	uint8_t payload[2];

	payload[0] = address & 0xFF;
	payload[1] = data;

	HAL_I2C_Master_Transmit(&hi2c1, devaddres, payload, sizeof(payload), 500);
	HAL_Delay(100);

	return;
}


int
eeprom_write_page (uint16_t address, uint8_t *data) {
	uint16_t devaddres = AT24_ADDR | ((address / 256) << 1);
	uint8_t payload[1 + EEPROM_PAGE];

	payload[0] = address & (~(EEPROM_PAGE - 1));
	memcpy(&(payload[1]), data, EEPROM_PAGE);

	HAL_I2C_Master_Transmit(&hi2c1, devaddres, payload, sizeof(payload), 500);
	HAL_Delay(100);

	return EEPROM_PAGE;
}
