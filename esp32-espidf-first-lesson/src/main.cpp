
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#define LED_OUT GPIO_NUM_16
#define BUTTON_IN GPIO_NUM_15

extern "C" void app_main() {
  while (1) {
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}
