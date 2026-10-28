#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"

void app_main() {
  while (1) {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(500);
  }
}
