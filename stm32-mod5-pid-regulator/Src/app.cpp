#include "app.h"
#include "console.hpp"
#include "etl/callback_timer_atomic.h"
#include "stm32f4xx_hal.h"
#include <etl/atomic.h>

namespace {
constexpr uint32_t BLINK_PERIOD_MS = 500;

using Timers = etl::callback_timer_atomic<2, etl::atomic_uint32_t>;
Timers timers;

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
} // namespace

extern "C" void app_setup(void) {
  console::add("ping", console::Handler::create<&pong>());

  auto blink_id =
      timers.register_timer(Timers::callback_type::create<&blink>(),
                            BLINK_PERIOD_MS, etl::timer::mode::REPEATING);

  timers.enable(true);
  timers.start(blink_id);
}

extern "C" void app_main(void) {
  while (true) {
    tick_timers();
    console::read_line();
  }
}
