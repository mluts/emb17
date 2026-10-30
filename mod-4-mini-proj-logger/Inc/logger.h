
#ifndef __LOGGER_H
#define __LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "at24c32.h"

#define AT24C32_A_PINS 0
#define EEPROM_START_ADDR 0x0000
#define EEPROM_PAGE_SIZE 32U
#define EEPROM_PAGE_COUNT 128U
#define LOG_MAX_COUNT 1000

#define LOG_STATUS_PAGE_ADDR                                                   \
  (EEPROM_START_ADDR + (EEPROM_PAGE_COUNT - 1) * EEPROM_PAGE_SIZE)
#define LOG_PAGE_COUNT (EEPROM_PAGE_COUNT - 1)
#define LOG_PAGE_ADDR(page)                                                    \
  (EEPROM_START_ADDR + (page % LOG_PAGE_COUNT) * EEPROM_PAGE_SIZE)

#define TAIL_PAGE(head_page)                                                   \
  (head_page >= LOG_PAGE_COUNT ? (head_page - LOG_PAGE_COUNT) : 0)

typedef uint16_t LogPage;

typedef struct {
  LogPage head_page;

  AT24C32_HandleTypeDef *eeprom;
  HAL_StatusTypeDef eeprom_is_ready;

} LoggerHandleDef;

typedef struct {
  LogPage log_page;
  char log[EEPROM_PAGE_SIZE - sizeof(uint16_t)];
} LogDef;

HAL_StatusTypeDef logger_init(LoggerHandleDef *logger,
                              AT24C32_HandleTypeDef *eeprom);
HAL_StatusTypeDef logger_read(LoggerHandleDef *logger, LogPage, LogDef *log);
HAL_StatusTypeDef logger_write(LoggerHandleDef *logger, const char *msg);
HAL_StatusTypeDef logger_log_tail_page(LoggerHandleDef *logger, LogPage *tail);

#ifdef __cplusplus
}
#endif

#endif /* __LOGGER_H */
