#pragma once

#include "stm32f4xx_hal.h" // IWYU pragma: keep
#include <cstdint>

// DRV833 behind inverting PC817 optocouplers, slow-decay PWM
// MCU -> HIGH -> LED on -> DRV input LOW. Compare 0 -> MCU LOW -> DRV HIGH
//   stop:    CCR1 = 0,    CCR2 = 0      both DRV inputs HIGH (brake)
//   forward: CCR1 = duty, CCR2 = 0      IN1 pulses LOW, IN2 = HIGH
//   reverse: CCR1 = 0,    CCR2 = duty   IN1 HIGH, IN2 pulses LOW

class Motor {

public:
  static constexpr int MAX_PERCENT = 100;

  void init(TIM_HandleTypeDef *htim);
  void set(int percent);

  int command() const { return command_; }

private:
  uint32_t duty(int percent) const; // compare vlaue for 0..100

  TIM_HandleTypeDef *tim_ = nullptr;

  int command_ = 0;
};
