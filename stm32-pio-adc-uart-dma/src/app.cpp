#include "app.h"
#include "main.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include <atomic>
#include <cstdint>
#include <etl/string.h>
#include <etl/to_string.h>
#include <stdio.h>

/*
  ADC is on PA1

  UART TX is on PA9
*/

// ADC handler
ADC_HandleTypeDef hadc;
// ADC DMA handler
DMA_HandleTypeDef hdma;

// UART handler
UART_HandleTypeDef huart;
// UART DMA handler
DMA_HandleTypeDef hdma_uart_tx;

static ADC_ChannelConfTypeDef sadc_channel_config = {0};
static GPIO_InitTypeDef sgpio_init = {0};

static uint32_t adc_val;
static std::atomic<bool> adc_data_ready;
static std::atomic<bool> adc_error;
static etl::string<32> tx_str;
static std::atomic<bool> uart_tx_done(true);
static std::atomic<bool> uart_error;

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

// Initialize UART Peripheral
static void UART_Init() {
  huart.Instance = USART1;
  huart.Init.BaudRate = 115200;
  huart.Init.WordLength = UART_WORDLENGTH_8B;
  huart.Init.StopBits = UART_STOPBITS_1;
  huart.Init.Parity = UART_PARITY_NONE;
  huart.Init.Mode = UART_MODE_TX;
  huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart.Init.OverSampling = UART_OVERSAMPLING_16;

  if (HAL_UART_Init(&huart) != HAL_OK) {
    Error_Handler();
  }
}

// Called from HAL_UART_Init
extern "C" void HAL_UART_MspInit(UART_HandleTypeDef *uartHandle) {

  __HAL_RCC_USART1_CLK_ENABLE();

  __HAL_RCC_GPIOA_CLK_ENABLE();

  // NOTE: PA9 = USART1_TX; no RX pin since huart.Init.Mode = UART_MODE_TX
  sgpio_init.Pin = GPIO_PIN_9;
  sgpio_init.Mode = GPIO_MODE_AF_PP;
  sgpio_init.Pull = GPIO_NOPULL;
  sgpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  sgpio_init.Alternate = GPIO_AF7_USART1;

  HAL_GPIO_Init(GPIOA, &sgpio_init);

  hdma_uart_tx.Instance = DMA2_Stream7;
  hdma_uart_tx.Init.Channel = DMA_CHANNEL_4;
  hdma_uart_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_uart_tx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_uart_tx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_uart_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_uart_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_uart_tx.Init.Mode = DMA_NORMAL;
  hdma_uart_tx.Init.Priority = DMA_PRIORITY_LOW;
  hdma_uart_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

  if (HAL_DMA_Init(&hdma_uart_tx) != HAL_OK) {
    Error_Handler();
  }

  __HAL_LINKDMA(uartHandle, hdmatx, hdma_uart_tx);

  HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
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

extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    uart_tx_done = true;
  }
}

extern "C" void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    uart_error = true;
  }
}

static void start_dma() {
  if (HAL_ADC_Start_DMA(&hadc, &adc_val, 1) != HAL_OK) {
    Error_Handler();
  }
}

static void send_uart(const etl::istring &str) {
  if (HAL_UART_Transmit_DMA(&huart,
                            reinterpret_cast<const uint8_t *>(str.data()),
                            static_cast<uint16_t>(str.size())) != HAL_OK) {
    Error_Handler();
  }
}

void app_setup(void) {
  DMA_Init();
  ADC_Init();
  UART_Init();

  // Delay to connect to console
  HAL_Delay(3000);
}

void app_loop(void) {
  // NOTE: lock-step ADC -> UART -> ADC; the next sample starts only once the
  // previous one has left the UART, so tx_str is never rewritten mid-transfer
  if (uart_tx_done.exchange(false)) {
    start_dma();
  }

  if (adc_data_ready.exchange(false)) {
    tx_str.clear();
    tx_str += "ADC PA1: ";
    // NOTE: append = true keeps the "ADC PA1: " prefix
    etl::to_string(adc_val, tx_str, true);
    tx_str += "\r\n";

    printf("%s", tx_str.c_str());
    send_uart(tx_str);
  }

  if (adc_error.exchange(false)) {
    printf("ADC error\r\n");
    uart_tx_done = true;
  }

  if (uart_error.exchange(false)) {
    printf("UART error\r\n");
    uart_tx_done = true;
  }
}
