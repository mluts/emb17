#pragma once

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_adc.h"
#include <cstdint>

class Setpoint {
public:
  static constexpr uint16_t ADC_MAX = 4095;
  static constexpr float MAX_DEGREES = 360.0f;
  static constexpr uint32_t TIMEOUT_MS = 10;

  void init(ADC_HandleTypeDef *hadc);
  void sample();
  float degrees();
  void set_override(float deg);
  void release();

private:
  ADC_HandleTypeDef *adc_ = nullptr;
  uint16_t raw_ = 0;
  float filtered_deg_ = 0.0;
  float override_deg_ = 0.0;
  bool override_ = false;
};
