#ifndef __APP_H
#define __APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

void USB_CDC_console_receive(uint8_t *Buf, uint32_t *Len);
void app_main(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_H */
