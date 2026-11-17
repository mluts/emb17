#include "pid.hpp"
#include <cmath>
#include <etl/algorithm.h>

Pid::Pid(float kp, float ki, float kd, float dt)
    : kp_(kp), ki_(ki), kd_(kd), dt_(dt) {}

void Pid::set_gains(float kp, float ki, float kd) {
  kp_ = kp;
  ki_ = ki;
  kd_ = kd;
  reset();
}

void Pid::set_deadband(float counts) { deadband_ = counts; }

void Pid::set_min_duty(float percent) { min_duty_ = percent; }

void Pid::reset() {
  integral_ = 0.0f;
  prev_meas_ = 0.0f;
  first_ = true;
}

float Pid::update(float setpoint, float measurement) {
  float error = setpoint - measurement;

  // Derivative on measurement, skipped on the first sample so a reset never
  // kicks
  float d = 0.0f;

  if (!first_) {
    d = -kd_ * (measurement - prev_meas_) / dt_;
  }
  prev_meas_ = measurement;
  first_ = false;

  if (std::fabs(error) <= deadband_) {
    return 0.0f; // freeze integral
  }

  integral_ += ki_ * error * dt_;
  integral_ = etl::clamp(integral_, -OUT_MAX, OUT_MAX);

  float out = etl::clamp(kp_ * error + integral_ + d, -OUT_MAX, OUT_MAX);

  if (out > 0.0f && out < min_duty_) {
    out = min_duty_;
  } else if (out < 0.0f && out > -min_duty_) {
    out = -min_duty_;
  }

  return out;
}
