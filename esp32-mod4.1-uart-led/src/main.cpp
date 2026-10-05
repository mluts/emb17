#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"

#define LED_GPIO      GPIO_NUM_16
#define BUTTON_GPIO   GPIO_NUM_0

#define UART_NUM      UART_NUM_1
#define UART_TX_PIN   GPIO_NUM_17  // -> STM32 PA10
#define UART_RX_PIN   GPIO_NUM_18  // <- STM32 PA9
#define UART_BAUD     115200
#define RX_BUF_SIZE   1024
#define TX_BUF_SIZE   256

constexpr char uart_msg[] = "hello stm32!\n";

bool get_key_button() {
  return gpio_get_level(BUTTON_GPIO) == 0;
}

void toggle_led() {
  static int level = 0;
  level = !level;
  gpio_set_level(LED_GPIO, level);
}

void init_led() {
  gpio_config_t cfg = {
      .pin_bit_mask = (1ULL << LED_GPIO),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&cfg));
}

void init_button() {
  gpio_config_t cfg = {
      .pin_bit_mask = (1ULL << BUTTON_GPIO),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&cfg));
}

void init_uart() {
  uart_config_t cfg = {
      .baud_rate = UART_BAUD,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .rx_flow_ctrl_thresh = 0,
      .source_clk = UART_SCLK_DEFAULT,
      .flags = {},
  };
  ESP_ERROR_CHECK(uart_driver_install(UART_NUM, RX_BUF_SIZE, TX_BUF_SIZE, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_param_config(UART_NUM, &cfg));
  ESP_ERROR_CHECK(
      uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE,
                   UART_PIN_NO_CHANGE));
}

extern "C" void app_main() {
  init_led();
  init_button();
  init_uart();

  printf("Starting...\n");

  bool last_button_state = 0, button_state = 0;
  uint8_t data[128];

  while (1) {
    int len = uart_read_bytes(UART_NUM, data, sizeof(data) - 1,
                              100 / portTICK_PERIOD_MS);
    if (len > 0) {
      data[len] = '\0';
      printf("received: %s\n", (char *)data);
      toggle_led();
    }

    button_state = get_key_button();
    if (button_state != last_button_state && button_state) {
      uart_write_bytes(UART_NUM, uart_msg, strlen(uart_msg));
    }
    last_button_state = button_state;
  }
}
