#include "encoder.hpp"

void Encoder::init(TIM_HandleTypeDef *htim) {
  tim_ = htim;
  HAL_TIM_Encoder_Start(tim_, TIM_CHANNEL_ALL);
  zero();
}

int32_t Encoder::read() {
  uint16_t cnt = static_cast<uint16_t>(__HAL_TIM_GET_COUNTER(tim_));
  // *unsigned* (1 - 65535) becomes *signed* 2
  int16_t delta = static_cast<int16_t>(cnt - last_cnt_);
  last_cnt_ = cnt;
  position_ += delta;
  return position_;
}

void Encoder::zero() {
  last_cnt_ = static_cast<uint16_t>(__HAL_TIM_GET_COUNTER(tim_));
  position_ = 0;
}

float Encoder::degrees() const { return position_ * DEGREES_PER_COUNT; }

int32_t Encoder::position() const { return position_; }
