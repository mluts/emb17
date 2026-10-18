#include <cstring>
#include <stdio.h>
#include "ds1307/ds1307.h"
#include "oled/oled.h"
// #include "i2c/i2c.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_i2c.h"
// #include "stm32f4xx_hal_def.h"

static constexpr OLED_SettingsTypeDef oled_settings = OLED_SETTINGS_128X64_DEFAULT();

class RTCClock {
private:
  OLED_HandleTypeDef *oled = NULL;
  I2C_HandleTypeDef *i2c = NULL;
  DS1307_HandleTypeDef *rtc = NULL;
  DS1307_DateTimeTypeDef datetime = {0};

public:
  RTCClock(I2C_HandleTypeDef *i2c) { this->i2c = i2c; }

  HAL_StatusTypeDef init(const DS1307_DateTimeTypeDef *dt) {
    HAL_StatusTypeDef status = HAL_OK;

    memcpy(&datetime, dt, sizeof(datetime));

    status = OLED_Init(i2c, &oled_settings, &oled);

    if (status != HAL_OK) {
      printf("[OLED] initialization failed (status=%d)\n", (int)status);
      return status;
    }

    status = DS1307_Init(i2c, &rtc);
    if (status != HAL_OK) {
      printf("[RTC] initialization failed (status=%d)\n", (int)status);
      return status;
    }

    bool running = false;
    status = DS1307_IsRunning(rtc, &running);
    if (status != HAL_OK) {
      printf("[RTC] oscillator check failed (status=%d)\n", (int)status);
      return status;
    }

    // Don't override state if runnin on battery
    if (!running) {
      status = DS1307_SetDateTime(rtc, &datetime);
      if (status != HAL_OK) {
        printf("[RTC] set datetime failed (status=%d)\n", (int)status);
        return status;
      }
    }


    return status;
  }

  HAL_StatusTypeDef clear() {
    return OLED_Clear(oled);
  }

  HAL_StatusTypeDef drawHeader(const char *txt) {
    return OLED_Print(oled, 0, 1, false, txt);
  }

  HAL_StatusTypeDef printDateTime() {
    HAL_StatusTypeDef status = HAL_OK;

    status = DS1307_GetDateTime(rtc, &datetime);

    if (status == HAL_OK) {
      OLED_Printf(oled, 2, 1, "%04u-%02u-%02u", (unsigned)datetime.year,
                  (unsigned)datetime.month, (unsigned)datetime.day);
      OLED_Printf(oled, 4, 2, "%02u:%02u:%02u", (unsigned)datetime.hour,
                  (unsigned)datetime.minute, (unsigned)datetime.second);
    }

    return status;
    }
};
