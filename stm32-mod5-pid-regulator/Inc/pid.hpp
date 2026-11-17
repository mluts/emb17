#pragma once

// Position PID on an error measured in counts.
//
// p = kp * err
// i = ki * err * dt, clamped to +-OUT_MAX so "I" alone never saturate
// d = -kd * (meas - prev_meas) / dt, derivative on measurement
// out = clamp(p + i + d, -OUT_MAX, OUT_MAX)
// |err| <= deadband -> out = 0, integral frozen
// 0 < |out| < min_duty -> |out| = min_duty (static friction kick)

class Pid {
public:
  static constexpr float OUT_MAX = 100.0f;

  Pid(float kp, float ki, float kd, float dt);

  void set_gains(float kp, float ki, float kd);
  void set_deadband(float counts);
  void set_min_duty(float percent);
  void reset();

  float update(float setpoint, float measurement);

  float kp() const { return kp_; }
  float ki() const { return ki_; }
  float kd() const { return kd_; }
  float deadband() const { return deadband_; }

private:
  float kp_, ki_, kd_, dt_;

  float deadband_ = 1.0f;
  float min_duty_ = 0.0f;

  float integral_ = 0.0f;
  float prev_meas_ = 0.0f;

  bool first_ = true; // no derivative on first sample
};
