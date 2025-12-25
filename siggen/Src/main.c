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
#include <stdio.h> // sprintf
#include "hal_plat.h" // HAL
#include "commands.h"
#include "instances.h"
#include "resource.h"
#include "parser.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

fifo_t* usart_stream;

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


void rf_pll_register_write (uint32_t r) {
	uint32_t n = u32_swap_endian(r); // max2871 needs big-endian
	HAL_GPIO_WritePin(PLL1_CS_GPIO_Port, PLL1_CS_Pin, GPIO_PIN_RESET); // LE low
	HAL_SPI_Transmit(&hspi2, (uint8_t*)&n, sizeof(n), 500); // Transmit 32 bits
	HAL_GPIO_WritePin(PLL1_CS_GPIO_Port, PLL1_CS_Pin, GPIO_PIN_SET); // LE high
}

int rf_pll_check_ld (void) {
	return HAL_GPIO_ReadPin(PLL1_LOCK_DETECT_GPIO_Port, PLL1_LOCK_DETECT_Pin);
}

void rf_pll_idle_wait (void) {
	HAL_GPIO_WritePin(PLL1_CS_GPIO_Port, PLL1_CS_Pin, GPIO_PIN_SET);
	HAL_Delay(30);
}

void attenuator_write (bda4700_t* instance, uint8_t n) {
	HAL_GPIO_WritePin(ATTENUATOR_CS_GPIO_Port, ATTENUATOR_CS_Pin, GPIO_PIN_RESET); // LE low
	HAL_SPI_Transmit(&hspi2, &n, sizeof(n), 500); // Transmit 8 bits
	HAL_GPIO_WritePin(ATTENUATOR_CS_GPIO_Port, ATTENUATOR_CS_Pin, GPIO_PIN_SET); // LE high
}


#define AT24C256_I2CADDR (0xA0)
#define AT24C256_PAGE (64)
#define AT24C256_MAX_PAGEADDRESS ((32768) / (AT24C256_PAGE))


int
at24c256_read_page (blockdevice_t* blockdevice, int pageaddress) {
	int ret = AT24C256_PAGE;
	HAL_StatusTypeDef rc;

	if (pageaddress >= AT24C256_MAX_PAGEADDRESS) {
		return 0;
	}
	uint8_t wpayload[2];

	wpayload[0] = ((pageaddress * AT24C256_PAGE) >> 8) & 0xFF; // MSB
	wpayload[1] = (pageaddress * AT24C256_PAGE) & 0xFF; // LSB
	rc = HAL_I2C_Master_Transmit(&hi2c1, AT24C256_I2CADDR, wpayload, sizeof(wpayload), 500);
	if (rc) {
		console_printf("i2c error 1 %i", rc);
		ret = 0;
	}
	rc = HAL_I2C_Master_Receive(&hi2c1, AT24C256_I2CADDR, (uint8_t*)blockdevice->buffer, AT24C256_PAGE, 500);
	if (rc) {
		console_printf("i2c error 2 %i", rc);
		ret = 0;
	}
	return ret;
}


int
at24c256_write_page (blockdevice_t* blockdevice, int pageaddress) {
	int ret = AT24C256_PAGE;
	HAL_StatusTypeDef rc;

	if (pageaddress >= AT24C256_MAX_PAGEADDRESS) {
		return 0;
	}
	uint8_t wpayload[2 + AT24C256_PAGE];

	wpayload[0] = ((pageaddress * AT24C256_PAGE) >> 8) & 0xFF; // MSB
	wpayload[1] = (pageaddress * AT24C256_PAGE) & 0xFF; // LSB
	memcpy(&(wpayload[2]), blockdevice->buffer, AT24C256_PAGE);

	rc = HAL_I2C_Master_Transmit(&hi2c1, AT24C256_I2CADDR, wpayload, sizeof(wpayload), 500);
	if (rc) {
		console_printf("i2c w error %i", rc);
		ret = 0;
	}
	delay_ms(100);

	return ret;
}


char get_online_char (void) {
	char c;
	fifo_pop_or_sleep(usart_stream, &c);
	return c;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  blockdevice_t *eeprom;

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  usart_stream = fifo_create(16, sizeof(char));
  if (!usart_stream) {
	  cpu_halt();
  }

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

  scheduler = scheduler_create();
  if (!scheduler) {
	  console_printf("scheduler init error");
	  cpu_halt();
  }

  // Variables, resources
  for (int i = 'z'; i >= 'a'; i--) {
	  char name[2];
	  name [0] = i; name[1] = '\0';
	  resource_add(name, NULL, variable_setter, variable_getter);
  }
  resource_add("freq", NULL, frequency_setter, frequency_getter);
  resource_add("level", NULL, rflevel_setter, rflevel_getter);
  resource_add("rnd", NULL, rnd_setter, rnd_getter);
  resource_add("ticks", NULL, void_setter, ticks_getter);


  // Main PLL instance
  rf_pll = max2871_create(rf_pll_register_write, rf_pll_check_ld, rf_pll_idle_wait);
  if (!rf_pll) {
	  console_printf("MAX2871 init error");
	  cpu_halt();
  }


  // Attenuator instance
  attenuator = bda4700_create(attenuator_write);
  if (!attenuator) {
  	  console_printf("Attenuator init error");
  	  cpu_halt();
  }

  // EEPROM instance
  eeprom = blockdevice_create(AT24C256_PAGE, AT24C256_MAX_PAGEADDRESS, at24c256_read_page, at24c256_write_page);
  if (!eeprom) {
	  console_printf("EEPROM init error");
	  cpu_halt();
  }
  //eeprom->read_block(eeprom, 0); // XXX This is to try to mitigate a rare I2C startup bug

  eepromfs = fs_create(eeprom);
  if (!eepromfs) {
  	  console_printf("FS init error");
  	  cpu_halt();
  }

  if (!fs_verify(eepromfs)) {
	  delay_ms(2000);
	  console_printf("m1:0x%04x m2:0x%04x entries:%i", eepromfs->params.magic1, eepromfs->params.magic2, eepromfs->params.rootdir_entries);
  	  console_printf("Formatting EEPROM");
  	  //fs_format(eepromfs, 16);
  }

  setup_commands();

  program = program_create(20, 80); // 20 lines, 80 characters each -> 1.6k max program size
  if (!program) {
  	  console_printf("program init error");
  	  cpu_halt();
  }

  ledflash(2);

  if (load_devicecfg()) {
  	  console_printf("config loaded");
  } else {
  	  set_rf_frequency(915000);
  	  set_rf_level(-20);
  	  set_rf_output(1);
  	  cfg_override();
  	  print_cfg();
  }

  program_ip = 0;
  program_run = 0;
  subroutine_sp = 0;

  if (load_autorun_program()) {
  	  console_printf("program loaded");
  	  if (!switchbreak()) {
  		  execute_program(program, NULL, NULL);
  	  }
  }

  online_input = terminal_input_create(get_online_char, program->header.fields.linelen - 2);
  if (!online_input) {
  	  console_printf("input init error");
  	  cpu_halt();
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  int chunks = t_chunks();
	  // Online command parser
	  parser_t* online_parser = parser_create(program->header.fields.linelen); // align to the program line length
	  if (!online_parser) {
		  console_printf("parser init error");
		  cpu_halt();
	  }
	  int prompt_pidx = 0;
	  char prompt[8];
	  sprintf(&(prompt[prompt_pidx]), "%c:> ", 'e');
	  // Interpreting and executing commands, till the eternity
	  cmd_line_parser(online_parser, terminal_get_line(online_input, prompt, 1), NULL, NULL);
	  parser_destroy(online_parser);

	  chunks -= t_chunks();
	  if (chunks) { // simple memory leak check, normally shouldn't ever happen
		  console_printf("t_malloc-t_free imbalance :%i", chunks);
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
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
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
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, ATTENUATOR_CS_Pin|PLL1_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : SWITCH_Pin */
  GPIO_InitStruct.Pin = SWITCH_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SWITCH_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ATTENUATOR_CS_Pin PLL1_CS_Pin */
  GPIO_InitStruct.Pin = ATTENUATOR_CS_Pin|PLL1_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PLL1_LOCK_DETECT_Pin */
  GPIO_InitStruct.Pin = PLL1_LOCK_DETECT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(PLL1_LOCK_DETECT_GPIO_Port, &GPIO_InitStruct);

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
