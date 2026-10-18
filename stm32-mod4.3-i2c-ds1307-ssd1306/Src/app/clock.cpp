#include <cstring>
#include <stdio.h>
#include "app/clock.h"
#include "ds1307/ds1307.h"
#include "oled/oled.h"
// #include "i2c/i2c.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_i2c.h"
// #include "stm32f4xx_hal_def.h"

static constexpr OLED_SettingsTypeDef oled_settings = OLED_SETTINGS_128X64_DEFAULT();

// NOTE: DS1307 weekday is 1 = Sunday ... 7 = Saturday
static const char *weekdayName(uint8_t weekday) {
  static const char *const names[] = {"Sun", "Mon", "Tue", "Wed",
                                      "Thu", "Fri", "Sat"};
  if ((weekday < 1) || (weekday > 7)) {
    return "???";
  }
  return names[weekday - 1];
}

void RTCClock::release() {
  if (rtc != NULL) {
    DS1307_Deinit(rtc);
    rtc = NULL;
  }
  if (oled != NULL) {
    OLED_Deinit(oled);
    oled = NULL;
  }
}

RTCClock::RTCClock(I2C_HandleTypeDef *i2c) { this->i2c = i2c; }

HAL_StatusTypeDef RTCClock::init(const DS1307_DateTimeTypeDef *dt) {
  HAL_StatusTypeDef status = HAL_OK;

  if ((oled != NULL) || (rtc != NULL)) {
    printf("[CLOCK] already initialized\n");
    return HAL_ERROR;
  }

  if (dt == NULL) {
    printf("[CLOCK] datetime is NULL\n");
    return HAL_ERROR;
  }

  memcpy(&datetime, dt, sizeof(datetime));

  status = OLED_Init(i2c, &oled_settings, &oled);

  if (status != HAL_OK) {
    printf("[OLED] initialization failed (status=%d)\n", (int)status);
    return status;
  }

  status = DS1307_Init(i2c, &rtc);
  if (status != HAL_OK) {
    printf("[CLOCK] initialization failed (status=%d)\n", (int)status);
    release();
    return status;
  }

  bool running = false;
  status = DS1307_IsRunning(rtc, &running);
  if (status != HAL_OK) {
    printf("[RTC] oscillator check failed (status=%d)\n", (int)status);
    release();
    return status;
  }

  // Don't override state if runnin on battery
  if (!running) {
    status = DS1307_SetDateTime(rtc, &datetime);
    if (status != HAL_OK) {
      printf("[RTC] set datetime failed (status=%d)\n", (int)status);
      release();
      return status;
    }
  }


  return status;
}

HAL_StatusTypeDef RTCClock::clear() {
  if (!oled) {
    printf("[OLED] not initialized\n");
    return HAL_ERROR;
  }

  return OLED_Clear(oled);
}

HAL_StatusTypeDef RTCClock::drawHeader(const char *txt) {
  if (!oled) {
    printf("[OLED] not initialized\n");
    return HAL_ERROR;
  }
  return OLED_Print(oled, 0, 1, false, txt);
}

HAL_StatusTypeDef RTCClock::updateDateTime() {
  if (!oled) {
    printf("[OLED] not initialized\n");
    return HAL_ERROR;
  }

  if (!rtc) {
    printf("[RTC] not initialized\n");
    return HAL_ERROR;
  }

  HAL_StatusTypeDef status = HAL_OK;

  status = DS1307_GetDateTime(rtc, &datetime);
  if (status != HAL_OK) {
    return status;
  }

  status = OLED_Printf(oled, 2, 1, "%s %02u.%02u.%04u",
                       weekdayName(datetime.weekday), (unsigned)datetime.day,
                       (unsigned)datetime.month, (unsigned)datetime.year);
  if (status != HAL_OK) {
    return status;
  }

  return OLED_Printf(oled, 4, 2, "%02u:%02u:%02u", (unsigned)datetime.hour,
                     (unsigned)datetime.minute, (unsigned)datetime.second);
}
