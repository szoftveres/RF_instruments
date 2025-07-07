#include "attenuator.h"
#include <stdlib.h> // malloc

attenuator_instance_t* attenuator_init (SPI_HandleTypeDef *spibus,
								        GPIO_TypeDef* LE_GPIO,
								        uint16_t LE_Pin) {

	attenuator_instance_t* instance = (attenuator_instance_t*) malloc(sizeof(attenuator_instance_t));
	if (!instance) {
		return instance;
	}
	instance->spi.spibus = spibus;
	instance->spi.LE_GPIO = LE_GPIO;
	instance->spi.LE_Pin = LE_Pin;

	// LE pin high
	HAL_GPIO_WritePin(instance->spi.LE_GPIO, instance->spi.LE_Pin, GPIO_PIN_SET);

	return instance;
}


void attenuator_write (attenuator_instance_t* instance, uint8_t n) {

	n &= 0x7F;
	uint8_t dout = 0;

	/* Turning the bits around (LSB first) */
	for (int i = 0; i != 8; i++) {
		dout = dout << 1;
		dout |= (n & 0x01);
		n = n >> 1;
	}

	HAL_GPIO_WritePin(instance->spi.LE_GPIO, instance->spi.LE_Pin, GPIO_PIN_RESET);

	HAL_SPI_Transmit(instance->spi.spibus, &dout, sizeof(dout), 500);

	HAL_GPIO_WritePin(instance->spi.LE_GPIO, instance->spi.LE_Pin, GPIO_PIN_SET);
}


int attenuator_set (attenuator_instance_t* instance, float dB) {
	uint8_t n = 0;

	if (dB < 0.0 || dB > 31.5) {
		return 0;
	}
	for (float atten = 0.0f; atten < dB; atten += 0.25) {
		n += 1;
	}
	attenuator_write(instance, n);
	return 1;
}

