
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#define LED_OUT GPIO_NUM_16
#define BUTTON_IN GPIO_NUM_15

extern "C" void app_main() {
  gpio_config_t gpio_led_conf = {0};
  gpio_led_conf.pin_bit_mask = 1ull << LED_OUT;
  gpio_led_conf.mode = GPIO_MODE_OUTPUT;
  gpio_led_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_led_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

  gpio_set_level(LED_OUT, 0);

  // disable interrupts
  gpio_led_conf.intr_type = GPIO_INTR_DISABLE;

  gpio_config(&gpio_led_conf);

  while (1) {
    gpio_set_level(LED_OUT, 1);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    gpio_set_level(LED_OUT, 0);
  }
}
