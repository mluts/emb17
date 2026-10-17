#include "main.h"
#include "printf/usb_printf.h" // IWYU pragma: keep
#include "stm32f4xx_hal.h"
#include <cstring>
#include <stdio.h>

#define I2C_TIMEOUT_MS 100
#define I2C_SPEED_HZ 100000U

extern "C" void main_cpp(void) {
  // Wait for serial interface setup
  HAL_Delay(5000);
  printf("Starting time display on SSD1306 using RTC module DS1307\n");

  while (1) {

    HAL_Delay(100);
  }
}
