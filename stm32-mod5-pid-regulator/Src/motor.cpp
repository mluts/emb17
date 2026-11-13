#include "motor.hpp"
#include "stm32f4xx_hal_tim.h"
#include <etl/algorithm.h>

// CCR - Capture/Compare Register of STM32 timer channel

void Motor::init(TIM_HandleTypeDef *htim) {
  tim_ = htim;
  __HAL_TIM_SET_COMPARE(tim_, TIM_CHANNEL_1, 0);
  __HAL_TIM_SET_COMPARE(tim_, TIM_CHANNEL_2, 0);

  HAL_TIM_PWM_Start(tim_, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(tim_, TIM_CHANNEL_2);
}

uint32_t Motor::duty(int precent) const {
  uint32_t period = __HAL_TIM_GET_AUTORELOAD(tim_) + 1;
  return period * static_cast<uint32_t>(precent) / 100;
}

void Motor::set(int percent) {
  command_ = etl::clamp(percent, -MAX_PERCENT, MAX_PERCENT);

  uint32_t ccr1 = command_ > 0 ? duty(command_) : 0;
  uint32_t ccr2 = command_ < 0 ? duty(-command_) : 0;

  __HAL_TIM_SET_COMPARE(tim_, TIM_CHANNEL_1, ccr1);
  __HAL_TIM_SET_COMPARE(tim_, TIM_CHANNEL_2, ccr2);
}
