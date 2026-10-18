#include <stdio.h>
#include "ds1307/ds1307.h"
#include "oled/oled.h"
// #include "i2c/i2c.h"
#include "stm32f4xx_hal_i2c.h"
// #include "stm32f4xx_hal_def.h"

static constexpr OLED_SettingsTypeDef oled_settings = OLED_SETTINGS_128X64_DEFAULT();

class Clock {
private:
  OLED_HandleTypeDef *oled = NULL;
  I2C_HandleTypeDef *i2c = NULL;
  DS1307_HandleTypeDef *rtc = NULL;

public:
  Clock(I2C_HandleTypeDef *i2c) { this->i2c = i2c; }
  
  HAL_StatusTypeDef init(const DS1307_DateTimeTypeDef dt) {
    HAL_StatusTypeDef status = HAL_OK;

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

    status = DS1307_SetDateTime(rtc, &dt);
    if (status != HAL_OK) {
      printf("[RTC] set datetime failed (status=%d)\n", (int)status);
      return status;
    }


    return status;
  }
};
