/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h> // memcpy
#include <unistd.h> // sbrk
#include <functions.h>
#include <parser.h>
#include "instances.h"
#include "resource.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

char usartbuf[USART_BUF_SIZE];
int usartbuf_ip;
int usartbuf_op;
int usartbuf_data;

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI2_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void halt_wait (void) {

	console_printf("System halted");
	while(1) {HAL_Delay(1950);}
}


void rf_pll_register_write (uint32_t r) {
	uint32_t n = u32_swap_endian(r); // max2871 needs big-endian
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET); // LE low
	HAL_SPI_Transmit(&hspi2, (uint8_t*)&n, sizeof(n), 500); // Transmit 32 bits
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET); // LE high
}

int rf_pll_check_ld (void) {
	return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8);
}

void rf_pll_idle_wait (void) {
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET);
	HAL_Delay(30);
}

void attenuator_write (bda4700_t* instance, uint8_t n) {
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET); // LE low
	HAL_SPI_Transmit(&hspi2, &n, sizeof(n), 500); // Transmit 8 bits
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET); // LE high
}


void usartbuf_push (char c) {
	if (((usartbuf_ip + 1) & (USART_BUF_SIZE - 1)) == usartbuf_op) {
		return;
	}
 	usartbuf[usartbuf_ip] = c;
 	usartbuf_ip = (usartbuf_ip + 1) & (USART_BUF_SIZE - 1);
 	usartbuf_data += 1;
}


int usartbuf_pop (char *c) {
	if (!usartbuf_data || usartbuf_ip == usartbuf_op) {
		return 0;
	}
 	usartbuf_data -= 1;
 	*c = usartbuf[usartbuf_op];
 	usartbuf_op = (usartbuf_op + 1) & (USART_BUF_SIZE - 1);
 	return 1;
}


#define AT24C256_I2CADDR (0xA0)
#define AT24C256_PAGE (64)
#define AT24C256_MAX_PAGEADDRESS ((32768) / (AT24C256_PAGE))

int
at24c256_read_page (blockdevice_t* blockdevice, int pageaddress) {
	if (pageaddress >= AT24C256_MAX_PAGEADDRESS) {
		return 0;
	}
	uint8_t wpayload[2];

	wpayload[0] = ((pageaddress * AT24C256_PAGE) >> 8) & 0xFF; // MSB
	wpayload[1] = (pageaddress * AT24C256_PAGE) & 0xFF; // LSB
	HAL_I2C_Master_Transmit(&hi2c1, AT24C256_I2CADDR, wpayload, sizeof(wpayload), 500);
	HAL_I2C_Master_Receive(&hi2c1, AT24C256_I2CADDR, (uint8_t*)blockdevice->buffer, AT24C256_PAGE, 500);
	return AT24C256_PAGE;
}


int
at24c256_write_page (blockdevice_t* blockdevice, int pageaddress) {
	if (pageaddress >= AT24C256_MAX_PAGEADDRESS) {
		return 0;
	}
	uint8_t wpayload[2 + AT24C256_PAGE];

	wpayload[0] = ((pageaddress * AT24C256_PAGE) >> 8) & 0xFF; // MSB
	wpayload[1] = (pageaddress * AT24C256_PAGE) & 0xFF; // LSB
	memcpy(&(wpayload[2]), blockdevice->buffer, AT24C256_PAGE);

	HAL_I2C_Master_Transmit(&hi2c1, AT24C256_I2CADDR, wpayload, sizeof(wpayload), 500);
	HAL_Delay(100);

	return AT24C256_PAGE;
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */


  usartbuf_ip = 0;
  usartbuf_op = 0;
  usartbuf_data = 0;

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);

  console_printf("");

  resources_setup(); // TODO error handling

  // Main PLL instance
  rf_pll = max2871_create(rf_pll_register_write, rf_pll_check_ld, rf_pll_idle_wait);
  if (!rf_pll) {
	  console_printf("MAX2871 init error");
	  halt_wait();
  }


  // Attenuator instance
  attenuator = bda4700_create(attenuator_write);
  if (!attenuator) {
  	  console_printf("Attenuator init error");
  	  halt_wait();
  }
  // EEPROM instance
  eeprom = blockdevice_create(AT24C256_PAGE, at24c256_read_page, at24c256_write_page);
  if (!attenuator) {
  	  console_printf("EEPROM init error");
  	  halt_wait();
  }
  program = program_create(14, AT24C256_PAGE); // 14 lines, 64 characters each
  if (!program) {
  	  console_printf("program init error");
  	  halt_wait();
  }
  for (int p = 0; p != direntries(); p++) {
	  directory[p].file = blockfile_create(eeprom, (p+1) * 0x10);
	  if (!directory[p].file) {
		  console_printf("programfile init error");
		  halt_wait();
	  }
  }
  if (program_load(program, directory[0].file)) { // try to load the 1st program
	  console_printf("program loaded");
  }
  if (load_devicecfg()) {
	  console_printf("config loaded");
  } else {
	  set_rf_frequency(915000);
	  set_rf_level(-20);
	  set_rf_output(1);
	  print_cfg();
  }

  program_ip = 0;
  program_run = 0;

  // Online command parser
  online_parser = parser_create(program->header.fields.linelen); // align to the program line length


  console_printf_e("> "); // initial prompt

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  char c;

	  // Go to sleep while waiting for an USART Rx interrupt
	  HAL_SuspendTick();
	  while (!usartbuf_pop(&c)) {
		  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
	  }
	  HAL_ResumeTick();

	  if (c == '\r') {
		  c = '\n';
	  }
	  console_printf_e("%c", c);  // echo
	  switch (c) {
		case '\n':
			parser_run(online_parser);
			console_printf_e("> ");
			break;
		case '\b':
			console_printf_e(" \b"); // delete
			if (parser_back(online_parser)) {
				console_printf_e(" "); // cannot move further back
				break;
			}
		default:
			if (parser_fill(online_parser, c)) {
				console_printf_e("\b"); // full line
			}
			break;
		}

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 12;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 38400;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6|GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin : PB14 */
  GPIO_InitStruct.Pin = GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PC6 PC12 */
  GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
