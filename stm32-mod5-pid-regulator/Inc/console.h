#ifndef __CONSOLE_H
#define __CONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void USBD_CDC_console_receive(uint8_t *Buf, uint32_t *Len);

#ifdef __cplusplus
}
#endif

#endif /* __CONSOLE_H */
