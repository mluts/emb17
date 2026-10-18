#include "ds1307/ds1307.h"
#include "main.h"
#include "printf/usb_printf.h" // IWYU pragma: keep
#include "stm32f4xx_hal.h"
#include <cstring>
#include <stdio.h>
#include "i2c/i2c.h"
#include "oled/oled.h"
#include "oled/oled_font.h"

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

  OLED_SettingsTypeDef oled_settings = OLED_SETTINGS_128X64_DEFAULT();
  OLED_HandleTypeDef *oled = NULL;

  RETURN_IF_NOT_OK(OLED_Init(&hi2c1, &oled_settings, &oled), "OLED",
                   "Initialization failed");

  DS1307_HandleTypeDef *rtc = NULL;
  RETURN_IF_NOT_OK(DS1307_Init(&hi2c1, &rtc), "RTC", "Initialization failed");

  const DS1307_DateTimeTypeDef initial_datetime = {.year = 2026,
                                                   .month = 10,
                                                   .day = 18,
                                                   .weekday = 7,
                                                   .hour = 13,
                                                   .minute = 25,
                                                   .second = 0};

  RETURN_IF_NOT_OK(DS1307_SetDateTime(rtc, &initial_datetime), "RTC",
                   "SetDateTime failed");

  OLED_Clear(oled);
  OLED_Print(oled, 0, 1, false, "DS1307 RTC");

  while (1) {
    DS1307_DateTimeTypeDef datetime;
    HAL_StatusTypeDef status;

    status = DS1307_GetDateTime(rtc, &datetime);

    if (status == HAL_OK) {
      OLED_Printf(oled, 2, 1, "%04u-%02u-%02u", (unsigned)datetime.year,
                  (unsigned)datetime.month, (unsigned)datetime.day);
      OLED_Printf(oled, 4, 2, "%02u:%02u:%02u", (unsigned)datetime.hour,
                  (unsigned)datetime.minute, (unsigned)datetime.second);
    } else {
      printf("[RTC] GetDateTime failed (status=%d)\n", status);
    }

    HAL_Delay(1000);
  }
}
