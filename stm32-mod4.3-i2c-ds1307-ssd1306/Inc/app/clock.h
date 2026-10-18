#ifndef CLOCK_H
#define CLOCK_H

#include "ds1307/ds1307.h"
#include "oled/oled.h"

class RTCClock {
private:
  OLED_HandleTypeDef *oled = NULL;
  I2C_HandleTypeDef *i2c = NULL;
  DS1307_HandleTypeDef *rtc = NULL;
  DS1307_DateTimeTypeDef datetime = {0};

  // NOTE: frees whatever init() allocated, so a failed init() can be retried
  void release();

public:
  RTCClock(I2C_HandleTypeDef *i2c);

  HAL_StatusTypeDef init(const DS1307_DateTimeTypeDef *dt);
  HAL_StatusTypeDef clear();
  HAL_StatusTypeDef drawHeader(const char *txt);
  HAL_StatusTypeDef updateDateTime();
};

#endif /* CLOCK_H */
