#include "app.h"
#include "console.hpp"
#include "encoder.hpp"
#include "etl/callback_timer_atomic.h"
#include "etl/format_spec.h"
#include "etl/string.h"
#include "etl/to_string.h"
#include "stm32f4xx_hal.h"
#include "tim.h"
#include <etl/atomic.h>
#include <etl/string_view.h>

namespace {
constexpr uint32_t BLINK_PERIOD_MS = 500;

using Timers = etl::callback_timer_atomic<2, etl::atomic_uint32_t>;
Timers timers;

Encoder encoder;

void blink() { HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); }

void pong(etl::string_view) { printf("\r\npong\r\n"); }

void tick_timers() {
  static uint32_t last = HAL_GetTick();

  uint32_t now = HAL_GetTick();

  if (now != last) {
    timers.tick(now - last);
    last = now;
  }
}

void enc(etl::string_view) {
  int32_t counts = encoder.read();
  etl::string<16> deg;

  etl::to_string(encoder.degrees(), deg, etl::format_spec().precision(1));

  printf("\r\nenc: %ld counts, %s degrees\r\n", static_cast<long>(counts),
         deg.c_str());
}

void zero(etl::string_view) { encoder.zero(); }
} // namespace

extern "C" void app_setup(void) {
  console::add("ping", console::Handler::create<&pong>());
  console::add("enc", console::Handler::create<&enc>());
  console::add("zero", console::Handler::create<&zero>());

  auto blink_id =
      timers.register_timer(Timers::callback_type::create<&blink>(),
                            BLINK_PERIOD_MS, etl::timer::mode::REPEATING);

  timers.enable(true);
  timers.start(blink_id);

  encoder.init(&htim4);
}

extern "C" void app_main(void) {
  while (true) {
    tick_timers();
    console::read_line();
  }
}
