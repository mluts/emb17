#include "at24c32.h"
#include "i2c.h"
#include "i2c_ext.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_i2c.h"
#include "usb_printf.h"
#include <console.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define BLINK_PERIOD_MS 500

// Shows found i2c devices
void i2c_scan() {

  HAL_StatusTypeDef status;

  /* Сканування шини I2C */
  uint8_t found[16];
  uint8_t found_count = 0;

  status = I2C_Scan(&hi2c1, found, sizeof(found), &found_count, 10);
  if (status != HAL_OK) {
    printf("[I2C] Scan failed (status=%d)\r\n", (int)status);
    return;
  }

  printf("[I2C] Scan: %u device(s) found\r\n", (unsigned)found_count);
  for (uint8_t i = 0; (i < found_count) && (i < sizeof(found)); i++) {
    printf("[I2C] Found device at 0x%02X\r\n", found[i]);
  }
}

// Simple fn for verifying that console works
void pong() { printf("\r\npong\r\n"); }

// Blink every BLINK_PERIOD_MS
void blink() {
  static uint32_t last_blink = 0;

  if (HAL_GetTick() - last_blink > BLINK_PERIOD_MS) {

    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    last_blink = HAL_GetTick();
  }
}

void app_setup() {
  console_add_command("ping", pong);
  console_add_command("i2c-scan", i2c_scan);
}

void app_main() {
  while (1) {
    blink();
    console_read_line();
  }
}
