#include "encoder/encoder.h"
#include "main.h"
#include "printf/usb_printf.h"
#include "pwm/pwm.h"
#include "servo/servo.h"
#include "sound/sound.h"
#include "stm32f4xx_hal.h"
#include <cstring>
#include <stdio.h>

const uint32_t UART_TIMEOUT_MS = 100;
constexpr char uart_msg[] = "hello esp32!\n";

bool get_key_button() {
  return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET;
}

extern "C" void main_cpp(void) {
  // Wait for serial interface setup
  HAL_Delay(1000);

  printf("Starting...\n");

  bool last_button_state = 0, button_state = 0;

  while (1) {
    button_state = get_key_button();

    if (button_state != last_button_state && button_state) {
      printf("button_pressed=%d\n", button_state);
      printf("sending UART msg: %s\n", uart_msg);
      HAL_UART_Transmit(&huart1, (uint8_t *)&uart_msg, strlen(uart_msg), UART_TIMEOUT_MS);
    }
    last_button_state = button_state;

    HAL_Delay(100);
  }
}
