#include "setpoint.hpp"
#include "stm32f4xx_hal_adc.h"

void Setpoint::init(ADC_HandleTypeDef *hadc) {
  adc_ = hadc;
  sample(); // seed conversion
  filtered_deg_ = raw_ * MAX_DEGREES / ADC_MAX; // start unfiltered
}

void Setpoint::sample() {
  HAL_ADC_Start(adc_);
  if (HAL_ADC_PollForConversion(adc_, TIMEOUT_MS) == HAL_OK) {
    raw_ = static_cast<uint16_t>(HAL_ADC_GetValue(adc_));
  }

  float raw_deg = raw_ * MAX_DEGREES / ADC_MAX;
  filtered_deg_ += (raw_deg - filtered_deg_) / 8;
}

float Setpoint::degrees() {
  if (override_) {
    return override_deg_;
  } else {
    return filtered_deg_;
  }
}

void Setpoint::set_override(float deg) {
  override_ = true;
  override_deg_ = deg;
}

void Setpoint::release() { override_ = false; }
