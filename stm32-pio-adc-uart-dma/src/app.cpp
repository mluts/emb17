#include "app.h"
#include "main.h"
#include "stm32f4xx_hal.h"
#include <atomic>
#include <cstdint>
#include <stdio.h>

ADC_HandleTypeDef hadc;
DMA_HandleTypeDef hdma;
static ADC_ChannelConfTypeDef sadc_channel_config = {0};

static uint32_t adc_val;
static std::atomic<bool> adc_data_ready;
static std::atomic<bool> adc_error;

static void DMA_Init() {
  // Don't forget to enable clock
  __HAL_RCC_DMA2_CLK_ENABLE();

  // ADC1 -> Stream0/CH0 OR Stream4/CH0
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

  // UART TX -> Stream7/CH4
  // UART RX -> Stream2/CH4 or Stream5/CH4
  HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
}

// ADC1 DR can only be read by DMA2
static void ADC_Init() {

  hadc.Instance = ADC1;
  hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;

  hadc.Init.ScanConvMode = DISABLE;
  hadc.Init.ContinuousConvMode = DISABLE;
  hadc.Init.DiscontinuousConvMode = DISABLE;

  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.NbrOfConversion = 1;
  hadc.Init.DMAContinuousRequests = DISABLE;
  hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

  if (HAL_ADC_Init(&hadc) != HAL_OK) {
    Error_Handler();
  }

  // Channel 1, Rank 1
  // Lowest possible `SamplingTime` (bigger source impedance -> more cycles)
  // ADC_CHANNEL 0..7 corresponds to PA0..PA7
  // ADC_CHANNEL 8, 9 corresponds to PB0, PB1
  // ADC_CHANNEL 10..15 corresponds to PC0..PC5

  // NOTE: GPIO PA1
  sadc_channel_config.Channel = ADC_CHANNEL_1;
  sadc_channel_config.Rank = 1;
  sadc_channel_config.SamplingTime = ADC_SAMPLETIME_3CYCLES;

  if (HAL_ADC_ConfigChannel(&hadc, &sadc_channel_config) != HAL_OK) {
    Error_Handler();
  }
}

// NOTE: Called by HAL_ADC_Init()
extern "C" void HAL_ADC_MspInit(ADC_HandleTypeDef *adcHandle) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_ADC1_CLK_ENABLE();

  // NOTE: GPIO PA1
  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  hdma.Instance = DMA2_Stream0;
  hdma.Init.Channel = DMA_CHANNEL_0;

  hdma.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma.Init.MemInc = DMA_MINC_ENABLE;
  hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  hdma.Init.Mode = DMA_NORMAL;
  hdma.Init.Priority = DMA_PRIORITY_LOW;
  hdma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

  if (HAL_DMA_Init(&hdma) != HAL_OK) {
    Error_Handler();
  }

  __HAL_LINKDMA(adcHandle, DMA_Handle, hdma);

  /* ADC1 interrupt Init */
  HAL_NVIC_SetPriority(ADC_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(ADC_IRQn);
}

extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
  if (hadc->Instance == ADC1) {
    adc_data_ready = true;
  }
}

extern "C" void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc) {
  if (hadc->Instance == ADC1) {
    adc_error = true;
  }
}

static void start_dma() {
  if (HAL_ADC_Start_DMA(&hadc, &adc_val, 1) != HAL_OK) {
    Error_Handler();
  }
}

void app_setup(void) {
  DMA_Init();
  ADC_Init();

  // Delay to connect to console
  HAL_Delay(3000);
}

void app_loop(void) {
  start_dma();

  if (adc_data_ready.exchange(false)) {
    printf("ADC PA1: %lu\n", static_cast<unsigned long>(adc_val));
  }
}
