// Host-side sanity check for Pid. Build and run with `./test` from the project
// root. NOTE: clangd reports missing <cassert> here because .clangd points at
// the ARM compilation database; the host build with the system c++ is fine.
#include "pid.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>

namespace {
constexpr float DT = 0.01f;

// Positive error drives positive, negative error drives negative
void test_output_sign() {
  Pid pid(1.0f, 0.0f, 0.0f, DT);
  assert(pid.update(10.0f, 0.0f) > 0.0f);
  pid.reset();
  assert(pid.update(0.0f, 10.0f) < 0.0f);
}

// |err| <= deadband yields exactly 0 and leaves the integral frozen
void test_deadband() {
  Pid pid(1.0f, 10.0f, 0.0f, DT);
  pid.set_deadband(1.0f);
  for (int i = 0; i < 100; ++i) {
    assert(pid.update(1.0f, 0.0f) == 0.0f);
  }
  // First sample outside the deadband: integral must start from zero,
  // so the output is P plus one tick of I, nothing accumulated before
  float out = pid.update(2.0f, 0.0f);
  assert(std::fabs(out - (2.0f + 10.0f * 2.0f * DT)) < 1e-4f);
}

// Pure I with a constant error saturates at OUT_MAX and no further
void test_integral_clamp() {
  Pid pid(0.0f, 1.0f, 0.0f, DT);
  float out = 0.0f;
  for (int i = 0; i < 100000; ++i) {
    out = pid.update(100.0f, 0.0f);
  }
  assert(out == Pid::OUT_MAX);
  // Reverse error: the integral must come back down in bounded time
  for (int i = 0; i < 20000; ++i) {
    out = pid.update(-100.0f, 0.0f);
  }
  assert(out == -Pid::OUT_MAX);
}

// Setpoint step with Kd set produces no derivative spike
void test_no_derivative_kick() {
  Pid pid(1.0f, 0.0f, 5.0f, DT);
  pid.update(0.0f, 0.0f);              // establishes prev_meas
  float out = pid.update(80.0f, 0.0f); // setpoint jumps, measurement does not
  assert(std::fabs(out - 80.0f) < 1e-4f); // P only
}

// Min-duty kick applies to small non-zero outputs in both directions
void test_min_duty() {
  Pid pid(1.0f, 0.0f, 0.0f, DT);
  pid.set_min_duty(15.0f);
  assert(pid.update(2.0f, 0.0f) == 15.0f);
  assert(pid.update(0.0f, 2.0f) == -15.0f);
  assert(pid.update(50.0f, 0.0f) == 50.0f); // above min duty: untouched
}
} // namespace

int main() {
  test_output_sign();
  test_deadband();
  test_integral_clamp();
  test_no_derivative_kick();
  test_min_duty();
  std::puts("pid_test: all passed");
  return 0;
}
