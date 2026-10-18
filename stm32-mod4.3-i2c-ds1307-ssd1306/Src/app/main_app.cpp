#include "main.h"
#include "printf/usb_printf.h" // IWYU pragma: keep
#include "stm32f4xx_hal.h"
#include <cstring>
#include <stdio.h>
#include "app/clock.h"
#include "i2c/i2c.h"

#define I2C_SPEED_HZ 100000U
#define DELAY_FOR_SERIAL_IFACE_MS 3000

// NOTE: don't use in `if () RETURN_IF_NOT_OK;`
#define RETURN_IF_NOT_OK(status, tag, msg)                                     \
  if ((status) != HAL_OK) {                                                    \
    printf("[" tag "] " msg " (status=%d)\r\n", (int)(status));                \
    return;                                                                    \
  }

static I2C_HandleTypeDef hi2c1;

extern "C" void main_cpp(void) {

  // Wait for serial interface setup
  HAL_Delay(DELAY_FOR_SERIAL_IFACE_MS);

  printf("Starting time display on SSD1306 using RTC module DS1307\n");

  RETURN_IF_NOT_OK(I2C_Init(&hi2c1, I2C1, I2C_SPEED_HZ), "I2C",
                   "I2C1 initialization failed");

  uint8_t found[16], found_count = 0;

  RETURN_IF_NOT_OK(I2C_Scan(&hi2c1, found, sizeof(found), &found_count, 10),
                   "I2C", "Scan failed");

  // NOTE: written to the DS1307 only when its oscillator is halted
  const DS1307_DateTimeTypeDef initial_datetime = {.year = 2026,
                                                   .month = 10,
                                                   .day = 18,
                                                   .weekday = 1,
                                                   .hour = 13,
                                                   .minute = 25,
                                                   .second = 0};

  RTCClock rtc_clock(&hi2c1);
  HAL_StatusTypeDef status;

  status = rtc_clock.init(&initial_datetime);
  RETURN_IF_NOT_OK(status, "CLOCK", "Initialization failed");

  status = rtc_clock.clear();
  RETURN_IF_NOT_OK(status, "CLOCK", "Clear failed");

  status = rtc_clock.drawHeader("DS1307 RTC");
  RETURN_IF_NOT_OK(status, "CLOCK", "Draw header failed");

  while (1) {
    status = rtc_clock.updateDateTime();
    if (status != HAL_OK) {
      printf("[CLOCK] update failed (status=%d)\r\n", (int)status);
    }

    HAL_Delay(1000);
  }
}
