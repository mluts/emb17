#include "at24c32.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_i2c.h"
#include "usb_printf.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define CONSOLE_BUF_SIZE 32
#define CONSOLE_LINE_SIZE 64

#define BLINK_PERIOD_MS 500

static struct {
  char buf[CONSOLE_BUF_SIZE];
  volatile uint32_t head; // producer-only
  volatile uint32_t tail; // consumer-only
} console_rb;

static bool rb_put(uint8_t c) {
  if (console_rb.head - console_rb.tail == CONSOLE_BUF_SIZE) { // full
    return false;
  }
  console_rb.buf[console_rb.head & (CONSOLE_BUF_SIZE - 1)] = c;
  __DMB();
  console_rb.head++;

  return true;
}

static bool rb_get(uint8_t *c) {
  if (console_rb.head == console_rb.tail) {
    return false;
  }

  *c = console_rb.buf[console_rb.tail & (CONSOLE_BUF_SIZE - 1)];
  __DMB();
  console_rb.tail++;

  return true;
}

typedef void (*ConsoleCommandFn)(void);

typedef struct {
  const char *cmd;
  ConsoleCommandFn f;
} ConsoleCommandDef;

void pong() { printf("\r\npong\r\n"); }

const ConsoleCommandDef console_commands[] = {{"ping", pong}};

void USB_CDC_console_receive(uint8_t *Buf, uint32_t *Len) {
  for (int i = 0; i < *Len; i++) {
    rb_put(Buf[i]);
  }
}

void console_run_cmd(char *line) {
  for (size_t i = 0; i < sizeof(console_commands) / sizeof(console_commands[0]);
       i++) {
    if (strcmp(line, console_commands[i].cmd) == 0) {
      console_commands[i].f();
      return;
    }
  }
  printf("\r\nUnknown command: %s\r\n", line);
}

void console_read_line() {
  static char line[CONSOLE_LINE_SIZE];
  static uint8_t c;
  static uint32_t len;
  static bool overflow;

  while (rb_get(&c)) {
    printf("%s", &c);
    if (c == '\r' || c == '\n') {
      if (overflow) {
        printf("error: line too long\r\n");
      } else {
        line[len] = '\0';

        console_run_cmd(line);

        len = 0;
      }
    } else if (len < sizeof(line) - 1) {
      line[len++] = c;
    } else {
      overflow = true;
    }
  }
}

void blink() {
  static uint32_t last_blink = 0;

  if (HAL_GetTick() - last_blink > BLINK_PERIOD_MS) {

    // printf("blink\r\n");
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    last_blink = HAL_GetTick();
  }
}

void app_main() {
  while (1) {
    blink();
    console_read_line();
  }
}
