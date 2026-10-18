#include "main.h"
#include "printf/usb_printf.h" // IWYU pragma: keep
#include "stm32f4xx_hal.h"
#include <cstring>
#include <stdio.h>
#include "i2c/i2c.h"

#define I2C_SPEED_HZ 100000U

// NOTE: don't use in `if () RETURN_IF_NOT_OK;`
#define RETURN_IF_NOT_OK(status, tag, msg)                                     \
  if ((status) != HAL_OK) {                                                    \
    printf("[" tag "] " msg " (status=%d)\r\n", (int)(status));                \
    return;                                                                    \
  }

static I2C_HandleTypeDef hi2c1;

extern "C" void main_cpp(void) {

  // Wait for serial interface setup
  HAL_Delay(5000);

  printf("Starting time display on SSD1306 using RTC module DS1307\n");

  RETURN_IF_NOT_OK(I2C_Init(&hi2c1, I2C1, I2C_SPEED_HZ), "I2C",
                   "I2C1 initialization failed");

  uint8_t found[16], found_count = 0;

  RETURN_IF_NOT_OK(I2C_Scan(&hi2c1, found, sizeof(found), &found_count, 10),
                   "I2C", "Scan failed");

  while (1) {
    HAL_Delay(1000);
  }
}
