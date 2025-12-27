/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SPI_CS1_Pin GPIO_PIN_3
#define SPI_CS1_GPIO_Port GPIOE
#define SPI_CS2_Pin GPIO_PIN_4
#define SPI_CS2_GPIO_Port GPIOE
#define GPIO4_Pin GPIO_PIN_8
#define GPIO4_GPIO_Port GPIOE
#define GPIO3_Pin GPIO_PIN_10
#define GPIO3_GPIO_Port GPIOE
#define GPIO2_Pin GPIO_PIN_12
#define GPIO2_GPIO_Port GPIOE
#define GPIO1_Pin GPIO_PIN_14
#define GPIO1_GPIO_Port GPIOE
#define SWITCH_Pin GPIO_PIN_10
#define SWITCH_GPIO_Port GPIOD
#define PUSHBUTTON_Pin GPIO_PIN_12
#define PUSHBUTTON_GPIO_Port GPIOD
#define LED_Pin GPIO_PIN_14
#define LED_GPIO_Port GPIOD
#define _SD_DETECT__Pin GPIO_PIN_6
#define _SD_DETECT__GPIO_Port GPIOC
#define SPI_CS3_Pin GPIO_PIN_1
#define SPI_CS3_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
