
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#include "driver/gpio.h"
#include "driver/gptimer.h"

#define LED_OUT GPIO_NUM_16
#define BUTTON_IN GPIO_NUM_15

static bool IRAM_ATTR timer_on_cb(gptimer_handle_t timer,
                                  const gptimer_alarm_event_data_t *edata,
                                  void *user_ctx) {
  return true;
}

extern "C" void app_main() {
  // Initialize LED GPIO
  gpio_config_t gpio_led_conf = {};
  gpio_led_conf.pin_bit_mask = 1ull << LED_OUT;
  gpio_led_conf.mode = GPIO_MODE_OUTPUT;

  // (good practice)
  // Disabling pullup/pulldown for OUTPUT
  gpio_led_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_led_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

  // disable interrupts
  gpio_led_conf.intr_type = GPIO_INTR_DISABLE;

  // Initialize BTN GPIO
  gpio_config_t gpio_btn_conf = {};
  gpio_btn_conf.pin_bit_mask = 1ull << BUTTON_IN;
  gpio_btn_conf.mode = GPIO_MODE_INPUT;
  gpio_btn_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_btn_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_btn_conf.intr_type = GPIO_INTR_DISABLE;

  // Configure LED and BTN GPIO
  gpio_config(&gpio_led_conf);
  gpio_config(&gpio_btn_conf);

  // Configure timer
  gptimer_handle_t gptimer = NULL;

  gptimer_config_t timer_config = {};
  timer_config.clk_src = GPTIMER_CLK_SRC_DEFAULT;
  timer_config.direction = GPTIMER_COUNT_UP;
  timer_config.resolution_hz = 1000; // 1kHz
  // timer_config.auto_reload = true; 

  gptimer_event_callbacks_t timer_callbacks = {};
  gptimer_register_event_callbacks(gptimer, &timer_callbacks, NULL);

  gptimer_alarm_config_t alarm_config = {};
  alarm_config.alarm_count = 1000;
  alarm_config.reload_count = 0;
  alarm_config.flags.auto_reload_on_alarm = false;

  gptimer_new_timer(&timer_config, &gptimer);

  gpio_set_level(LED_OUT, 0);

  while (1) {
    int btn_state = gpio_get_level(BUTTON_IN);

    if (btn_state == 1) {
      gpio_set_level(LED_OUT, 0);
    } else {
      gpio_set_level(LED_OUT, 1);
    }

    // vTaskDelay(200 / portTICK_PERIOD_MS);
    vTaskDelay(100);
    // gpio_set_level(LED_OUT, 0);
    // vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}
