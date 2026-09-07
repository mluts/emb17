#include "stm32f4xx_hal.h"
#include <stdbool.h>
// #include <stdio.h>

extern "C" void main_cpp(void) {
  while (1) {

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    HAL_Delay(1000);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    HAL_Delay(1000);
  }
}
