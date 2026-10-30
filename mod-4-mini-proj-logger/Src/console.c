#include "console.h"
#include "stm32f4xx_hal.h"
#include "usb_printf.h"
#include <stdbool.h>
#include <stdint.h>

#define CONSOLE_BUF_SIZE 32
#define CONSOLE_LINE_SIZE 64

static struct {
  char buf[CONSOLE_BUF_SIZE];
  volatile uint32_t head; // producer-only
  volatile uint32_t tail; // consumer-only
} console_rb;

static bool console_rb_put(uint8_t c) {
  if (console_rb.head - console_rb.tail == CONSOLE_BUF_SIZE) { // full
    return false;
  }
  console_rb.buf[console_rb.head & (CONSOLE_BUF_SIZE - 1)] = c;
  __DMB();
  console_rb.head++;

  return true;
}

static bool console_rb_get(uint8_t *c) {
  if (console_rb.head == console_rb.tail) {
    return false;
  }

  *c = console_rb.buf[console_rb.tail & (CONSOLE_BUF_SIZE - 1)];
  __DMB();
  console_rb.tail++;

  return true;
}

static uint32_t console_commands_len = 0;
static ConsoleCommandDef console_commands[32];

void USBD_CDC_console_receive(uint8_t *Buf, uint32_t *Len) {
  for (int i = 0; i < *Len; i++) {
    console_rb_put(Buf[i]);
  }
}

static void console_run_cmd(char *line) {
  for (size_t i = 0; i < sizeof(console_commands) / sizeof(console_commands[0]);
       i++) {
    if (strcmp(line, console_commands[i].cmd) == 0) {
      console_commands[i].f();
      return;
    }
  }
  printf("\r\nUnknown command: \"%s\"\r\n", line);
}

///
/// @brief Read console line and run first matching command fn
///
/// @return Result of operation: true if newline, otherwise false
///
bool console_read_line() {
  static char line[CONSOLE_LINE_SIZE];
  static uint8_t c;
  static uint32_t len;
  static bool overflow;

  while (console_rb_get(&c)) {

    if (overflow) {
      printf("error: line too long\r\n");
    }

    if (c == '\r' || c == '\n') {
      line[len] = '\0';

      console_run_cmd(line);

      len = 0;
      overflow = false;
    } else if (len < sizeof(line) - 1) {
      line[len++] = c;
    } else {
      overflow = true;
    }
  }

  if (len == 0) {
    return true;
  } else {
    return false;
  }
}

///
/// @brief Add new command to be run
///
/// @param cmd: string matched against console input
/// @param f: function to be called when string matched against input
///
void console_add_command(const char *cmd, ConsoleCommandFn f) {
  console_commands[console_commands_len++] = (ConsoleCommandDef){cmd, f};
}
