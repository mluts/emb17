#ifndef __CONSOLE_H
#define __CONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef void (*ConsoleCommandFn)(const char *);

typedef struct {
  const char *cmd;
  ConsoleCommandFn f;
} ConsoleCommandDef;

bool console_read_line();
void console_add_command(const char *cmd, ConsoleCommandFn f);
void USBD_CDC_console_receive(uint8_t *Buf, uint32_t *Len);

#ifdef __cplusplus
}
#endif

#endif /* __CONSOLE_H */
