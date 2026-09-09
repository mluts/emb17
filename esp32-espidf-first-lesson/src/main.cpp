
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#define LED_OUT GPIO_NUM_16
#define BUTTON_IN GPIO_NUM_15

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
