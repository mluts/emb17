#pragma once

#include "stm32f4xx_hal.h"
#include <cstdint>

class Encoder {
public:
  // KY-040: 20 detent per turn, TI12 mode counts all 4 edges per detent
  // More counts per turn give smoother, more informative error and derivative
  // at the same 100Hz tick rate.
  static constexpr int32_t COUNTS_PER_TURN = 80;
  static constexpr float DEGREES_PER_COUNT = 360.0f / COUNTS_PER_TURN;
  void init(TIM_HandleTypeDef *htim);

  int32_t read();
  void zero();
  float degrees() const;
  int32_t position() const;

private:
  TIM_HandleTypeDef *tim_ = nullptr;

  int32_t position_ = 0;
  uint16_t last_cnt_ = 0;
};
