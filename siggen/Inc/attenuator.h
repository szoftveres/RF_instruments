#ifndef __ATTENUATOR__H_
#define __ATTENUATOR__H_

#include <stdint.h>
#include "stm32f4xx_hal.h" // hal types


typedef struct {
	struct {
		SPI_HandleTypeDef *spibus;
		GPIO_TypeDef* LE_GPIO;
		uint16_t LE_Pin;
	} spi;

} attenuator_instance_t;

attenuator_instance_t* attenuator_init (SPI_HandleTypeDef *spibus,
								        GPIO_TypeDef* LE_GPIO,
								        uint16_t LE_Pin);

int attenuator_set (attenuator_instance_t* instance, float dB);


#endif
