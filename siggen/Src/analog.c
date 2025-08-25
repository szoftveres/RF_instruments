#include "analog.h"
#include "stm32f4xx_hal.h"
#include <stdlib.h> //malloc


extern TIM_HandleTypeDef htim2;
extern DAC_HandleTypeDef hdac;
extern ADC_HandleTypeDef hadc1;



static int* samplebuf;
static int* samplebuf_ptr;
static int buf_len;


static uint32_t phaseshift;
static uint32_t phaseaccumulator;


int set_TIM_reload_frequency (int fs) {
    /* PCLK1 prescaler equal to 1 => TIMCLK = PCLK1 */
    /* PCLK1 prescaler different from 1 => TIMCLK = 2 * PCLK1 */
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


int* analog_get_buf (IRQn_Type irq) {
	int* p;
	HAL_NVIC_DisableIRQ(irq);
	while (!(p = samplebuf_ptr)) {
		HAL_NVIC_EnableIRQ(irq);
		HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
		HAL_NVIC_DisableIRQ(irq);
	}
	samplebuf_ptr = NULL;
	HAL_NVIC_EnableIRQ(irq);
	return p;
}


int* dac_get_buf (void) {
	return analog_get_buf(DMA1_Stream6_IRQn);
}

int* adc_get_buf (void) {
	return analog_get_buf(DMA2_Stream0_IRQn);
}


void HAL_ADC_ConvCpltCallback (ADC_HandleTypeDef *hadc) {
	samplebuf_ptr = &(samplebuf[buf_len]);
}

void HAL_ADC_ConvHalfCpltCallback (ADC_HandleTypeDef *hadc) {
	samplebuf_ptr = &(samplebuf[0]);
}


void HAL_DACEx_ConvCpltCallbackCh2 (DAC_HandleTypeDef *hdac) {
	samplebuf_ptr = &(samplebuf[buf_len]);
}


void HAL_DACEx_ConvHalfCpltCallbackCh2 (DAC_HandleTypeDef *hdac) {
	samplebuf_ptr = &(samplebuf[0]);
}

int analog_setup (int fs, int fc, int buflen) {
	samplebuf = (int*) malloc(sizeof(int) * buf_len * 2);
	if (!samplebuf) {
		return 0;
	}
	for (int i = 0; i != buf_len * 2; i++) {
		samplebuf[i] = 2048;
	}
	samplebuf_ptr = NULL;
	set_TIM_reload_frequency(fs);
	phaseaccumulator = 0;
	phaseshift = (uint32_t)((268435456.0f / ((float)(fs / 16))) * (float)fc);
	return buflen;
}

int dac_on (int fs, int fc, int buflen) {
	buf_len = analog_setup(fs, fc, buflen);

	HAL_SuspendTick();
	HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_2, (uint32_t*)samplebuf, buf_len * 2, DAC_ALIGN_12B_R);
	HAL_TIM_Base_Start(&htim2);
	return 1;
}

void dac_off (void) {

	dac_get_buf(); // flush
	HAL_TIM_Base_Stop(&htim2);
	HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_2);
	HAL_ResumeTick();
	free(samplebuf);
}


int adc_on (int fs, int fc, int buflen) {
	buf_len = analog_setup(fs, fc, buflen);

	HAL_SuspendTick();
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)samplebuf, buf_len * 2);
	HAL_TIM_Base_Start(&htim2);
	return 1;
}

void adc_off (void) {

	adc_get_buf(); // flush
	HAL_TIM_Base_Stop(&htim2);
	HAL_ADC_Stop_DMA(&hadc1);
	HAL_ResumeTick();
	free(samplebuf);
}


#define LOOKUP_LEN (256)


const int sinewave_256[LOOKUP_LEN] = {
          0,         50,        100,        151,
        201,        251,        300,        350,
        399,        449,        497,        546,
        594,        642,        690,        737,
        783,        830,        875,        920,
        965,       1009,       1052,       1095,
       1137,       1179,       1219,       1259,
       1299,       1337,       1375,       1411,
       1447,       1483,       1517,       1550,
       1582,       1614,       1644,       1674,
       1702,       1729,       1756,       1781,
       1805,       1828,       1850,       1871,
       1891,       1910,       1927,       1944,
       1959,       1973,       1986,       1997,
       2008,       2017,       2025,       2032,
       2037,       2041,       2045,       2046,
       2047,       2046,       2045,       2041,
       2037,       2032,       2025,       2017,
       2008,       1997,       1986,       1973,
       1959,       1944,       1927,       1910,
       1891,       1871,       1850,       1828,
       1805,       1781,       1756,       1729,
       1702,       1674,       1644,       1614,
       1582,       1550,       1517,       1483,
       1447,       1411,       1375,       1337,
       1299,       1259,       1219,       1179,
       1137,       1095,       1052,       1009,
        965,        920,        875,        830,
        783,        737,        690,        642,
        594,        546,        497,        449,
        399,        350,        300,        251,
        201,        151,        100,         50,
          0,        -50,       -100,       -151,
       -201,       -251,       -300,       -350,
       -399,       -449,       -497,       -546,
       -594,       -642,       -690,       -737,
       -783,       -830,       -875,       -920,
       -965,      -1009,      -1052,      -1095,
      -1137,      -1179,      -1219,      -1259,
      -1299,      -1337,      -1375,      -1411,
      -1447,      -1483,      -1517,      -1550,
      -1582,      -1614,      -1644,      -1674,
      -1702,      -1729,      -1756,      -1781,
      -1805,      -1828,      -1850,      -1871,
      -1891,      -1910,      -1927,      -1944,
      -1959,      -1973,      -1986,      -1997,
      -2008,      -2017,      -2025,      -2032,
      -2037,      -2041,      -2045,      -2046,
      -2047,      -2046,      -2045,      -2041,
      -2037,      -2032,      -2025,      -2017,
      -2008,      -1997,      -1986,      -1973,
      -1959,      -1944,      -1927,      -1910,
      -1891,      -1871,      -1850,      -1828,
      -1805,      -1781,      -1756,      -1729,
      -1702,      -1674,      -1644,      -1614,
      -1582,      -1550,      -1517,      -1483,
      -1447,      -1411,      -1375,      -1337,
      -1299,      -1259,      -1219,      -1179,
      -1137,      -1095,      -1052,      -1009,
       -965,       -920,       -875,       -830,
       -783,       -737,       -690,       -642,
       -594,       -546,       -497,       -449,
       -399,       -350,       -300,       -251,
       -201,       -151,       -100,        -50
};


void dds_next_sample (int *i, int *q) {

	*i = sinewave_256[(phaseaccumulator) >> 24];
	*q = sinewave_256[(phaseaccumulator + 0x40000000) >> 24];

	phaseaccumulator += phaseshift;
}
