#include "app.h"
#include "adc.h"
#include "console.hpp"
#include "encoder.hpp"
#include "etl/callback_timer_atomic.h"
#include "etl/format_spec.h"
#include "etl/string.h"
#include "etl/to_string.h"
#include "motor.hpp"
#include "pid.hpp"
#include "setpoint.hpp"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"
#include "tim.h"
#include <etl/atomic.h>
#include <etl/string_view.h>
#include <etl/to_arithmetic.h>

namespace {
constexpr uint32_t BLINK_PERIOD_MS = 500;
constexpr uint32_t LOG_PERIOD_MS = 10;
etl::timer::id::type log_id = etl::timer::id::NO_TIMER;

constexpr float LOOP_DT = 0.01f;
constexpr float KP_DEFAULT = 30.0f;
constexpr float KI_DEFAULT = 400.0f;
constexpr float KD_DEFAULT = 1.5f;

using Timers = etl::callback_timer_atomic<2, etl::atomic_uint32_t>;
Timers timers_controller;

Encoder encoder;
Motor motor;
Setpoint setpoint;
Pid pid(KP_DEFAULT, KI_DEFAULT, KD_DEFAULT, LOOP_DT);

bool parse_float(etl::string_view arg, float &out) {
  auto v = etl::to_arithmetic<float>(arg);

  if (!v.has_value()) {
    printf("\r\nerror: not a number\r\n");
    return false;
  }

  out = v.value();
  return true;
}

void print_gains() {
  etl::string<16> kp, ki, kd, db, md;
  auto spec = etl::format_spec().precision(3);

  etl::to_string(pid.kp(), kp, spec);
  etl::to_string(pid.ki(), ki, spec);
  etl::to_string(pid.kd(), kd, spec);
  etl::to_string(pid.deadband(), db, spec);
  etl::to_string(pid.min_duty(), md, spec);

  printf("\r\nkp %s ki %s kd %s deadband %s minduty %s\r\n", kp.c_str(),
         ki.c_str(), kd.c_str(), db.c_str(), md.c_str());
}

struct ControlState {
  volatile bool loop_enabled = false;
  volatile uint32_t ticks = 0;
  volatile float setpoint_deg = 0.0f;
  volatile int32_t counts = 0;
  volatile float output = 0.0f;
};

ControlState state;

/// @brief Blinks LED on BlackPill board
void blink() { HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); }

/// @brief Util to test that console works
void pong(etl::string_view) { printf("\r\npong\r\n"); }

/// @brief This enables `timers_controller`
void tick_timers() {
  static uint32_t last = HAL_GetTick();

  uint32_t now = HAL_GetTick();

  if (now != last) {
    timers_controller.tick(now - last);
    last = now;
  }
}

void enc(etl::string_view) {
  int32_t counts = encoder.position();

  etl::string<16> deg;

  etl::to_string(encoder.degrees(), deg, etl::format_spec().precision(1));

  printf("\r\nenc: %ld counts, %s degrees\r\n", static_cast<long>(counts),
         deg.c_str());
}

void zero(etl::string_view) { encoder.zero(); }

void motor_cmd(etl::string_view arg) {
  auto value = etl::to_arithmetic<int>(arg);

  if (!value.has_value()) {
    printf("\r\nerror: not a number\r\n");
    return;
  }

  state.loop_enabled = false;

  motor.set(value.value());
  printf("\r\nmotor %d\r\n", motor.command());
}

void adc_cmd(etl::string_view) {
  etl::string<16> filt, deg;
  etl::to_string(setpoint.filtered(), filt, etl::format_spec().precision(1));
  etl::to_string(setpoint.degrees(), deg, etl::format_spec().precision(1));

  printf("\r\nadc: raw %u, filtered %s, setpoint %s degrees\r\n",
         setpoint.raw(), filt.c_str(), deg.c_str());
}

void set_deg(etl::string_view arg) {
  float deg = 0;

  if (parse_float(arg, deg)) {
    setpoint.set_override(deg);
  }
}

void pot(etl::string_view) { setpoint.release(); }

void kp_cmd(etl::string_view arg) {
  float v;
  if (parse_float(arg, v)) {
    pid.set_gains(v, pid.ki(), pid.kd());
    print_gains();
  }
}
void ki_cmd(etl::string_view arg) {
  float v;
  if (parse_float(arg, v)) {
    pid.set_gains(pid.kp(), v, pid.kd());
    print_gains();
  }
}

void kd_cmd(etl::string_view arg) {
  float v;
  if (parse_float(arg, v)) {
    pid.set_gains(pid.kp(), pid.ki(), v);
    print_gains();
  }
}

void deadband_cmd(etl::string_view arg) {
  float v;
  if (parse_float(arg, v)) {
    pid.reset();
    pid.set_deadband(v);
    print_gains();
  }
}

void minduty_cmd(etl::string_view arg) {
  float v;
  if (parse_float(arg, v)) {
    pid.reset();
    pid.set_min_duty(v);
    print_gains();
  }
}

void loop_cmd(etl::string_view arg) {
  if (arg == etl::string_view("on")) {
    pid.reset();
    state.loop_enabled = true;
  } else if (arg == etl::string_view("off")) {
    state.loop_enabled = false;
    motor.set(0);
  } else {
    printf("\r\nerror: loop on|off\r\n");
    return;
  }

  printf("\r\nloop %s\r\n", state.loop_enabled ? "on" : "off");
}

void log_cmd(etl::string_view arg) {
  if (arg == etl::string_view("on")) {
    // printf("\r\nt_ms sp_deg pv_deg err_cnt out_pct\r\n");
    timers_controller.start(log_id);
  } else if (arg == etl::string_view("off")) {
    timers_controller.stop(log_id);
  } else {
    printf("\r\nerror: log on|off\r\n");
    return;
  }
}
} // namespace

extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance != TIM10) {
    return;
  }

  state.ticks = state.ticks + 1;

  setpoint.sample();

  int32_t counts = encoder.read();

  if (!state.loop_enabled) {
    return;
  }

  float sp_deg = setpoint.degrees();
  // Converting degrees to encoder pulses
  float out = pid.update(sp_deg / Encoder::DEGREES_PER_COUNT,
                         static_cast<float>(counts));
  motor.set(static_cast<int>(out));

  state.setpoint_deg = sp_deg;
  state.counts = counts;
  state.output = out;
}

void log_line() {
  float sp = setpoint.degrees();
  float out = state.output;
  long err =
      static_cast<long>(sp / Encoder::DEGREES_PER_COUNT) - encoder.position();
  etl::string<16> sp_str, enc_deg_str, out_str;

  auto spec = etl::format_spec().precision(1);

  etl::to_string(sp, sp_str, spec);
  etl::to_string(encoder.degrees(), enc_deg_str, spec);
  etl::to_string(out, out_str, spec);

  printf("%lu %s (sp) %s (enc) %ld (err) %s (out)\r\n", static_cast<unsigned long>(HAL_GetTick()),
         sp_str.c_str(), enc_deg_str.c_str(), err, out_str.c_str());
}

extern "C" void app_setup(void) {
  console::add("ping", console::Handler::create<&pong>());
  console::add("enc", console::Handler::create<&enc>());
  console::add("zero", console::Handler::create<&zero>());
  console::add("motor", console::Handler::create<&motor_cmd>());
  console::add("adc", console::Handler::create<&adc_cmd>());
  console::add("set", console::Handler::create<&set_deg>());
  console::add("pot", console::Handler::create<&pot>());

  console::add("kp", console::Handler::create<&kp_cmd>());
  console::add("ki", console::Handler::create<&ki_cmd>());
  console::add("kd", console::Handler::create<&kd_cmd>());
  console::add("deadband", console::Handler::create<&deadband_cmd>());
  console::add("minduty", console::Handler::create<&minduty_cmd>());
  console::add("loop", console::Handler::create<&loop_cmd>());
  console::add("log", console::Handler::create<&log_cmd>());

  auto blink_id = timers_controller.register_timer(
      Timers::callback_type::create<&blink>(), BLINK_PERIOD_MS,
      etl::timer::mode::REPEATING);

  log_id = timers_controller.register_timer(
      Timers::callback_type::create<&log_line>(), LOG_PERIOD_MS,
      etl::timer::mode::REPEATING);

  timers_controller.enable(true);
  timers_controller.start(blink_id);

  pid.set_min_duty(Motor::MIN_DUTY);

  encoder.init(&htim4);
  motor.init(&htim1);
  setpoint.init(&hadc1);

  HAL_TIM_Base_Start_IT(&htim10);
}

extern "C" void app_main(void) {
  while (true) {
    tick_timers();
    console::read_line();
  }
}
