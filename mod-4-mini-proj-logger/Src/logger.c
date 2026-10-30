#include "logger.h"
#include "at24c32.h"
#include "i2c.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_i2c.h"
#include "usbd_def.h"
#include <stdint.h>

// static LoggerHandleDef eeprom_rb;

typedef struct {
  LogPage head_page;
} LogStatusDef;

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif /* MIN */

#define HAL_GUARD(status)                                                      \
  if (status != HAL_OK) {                                                      \
    return status;                                                             \
  }

static HAL_StatusTypeDef save_rb_state(LoggerHandleDef *logger) {
  LogStatusDef log_status = {0};

  HAL_GUARD(logger->eeprom_is_ready);

  log_status.head_page = logger->head_page;

  return AT24C32_Write(logger->eeprom, LOG_STATUS_PAGE_ADDR,
                       (uint8_t *)&log_status, sizeof(log_status));
}

static HAL_StatusTypeDef restore_rb_state(LoggerHandleDef *logger) {
  LogStatusDef log_status = {0};
  HAL_StatusTypeDef status = 0;

  HAL_GUARD(logger->eeprom_is_ready);

  status = AT24C32_Read(logger->eeprom, LOG_STATUS_PAGE_ADDR,
                        (uint8_t *)&log_status, sizeof(log_status));

  HAL_GUARD(status);

  if (log_status.head_page < LOG_MAX_COUNT) {
    logger->head_page = log_status.head_page;
  }

  return status;
}

/// @brief Initialize addresses from zero
HAL_StatusTypeDef logger_init(LoggerHandleDef *logger,
                              AT24C32_HandleTypeDef *eeprom) {

  logger->eeprom = eeprom;
  logger->head_page = 0;

  HAL_GUARD(logger->eeprom_is_ready);

  restore_rb_state(logger);

  return logger->eeprom_is_ready;
}

HAL_StatusTypeDef logger_read(LoggerHandleDef *logger, LogPage log_number,
                              LogDef *log) {
  HAL_GUARD(logger->eeprom_is_ready);

  return AT24C32_Read(logger->eeprom, LOG_PAGE_ADDR(log_number), (uint8_t *)log,
                      sizeof(LogDef));
}

HAL_StatusTypeDef logger_write(LoggerHandleDef *logger, const char *msg) {
  HAL_GUARD(logger->eeprom_is_ready);

  static HAL_StatusTypeDef status;
  LogDef log = {
      .log_page = logger->head_page,
  };

  strncpy((char *)log.log, msg, sizeof(log.log) - 2);
  log.log[sizeof(log.log)-1] = '\0';

  status = AT24C32_Write(logger->eeprom, LOG_PAGE_ADDR(logger->head_page),
                         (uint8_t *)&log, sizeof(LogDef));

  HAL_GUARD(status);

  logger->head_page = (logger->head_page + 1) % LOG_MAX_COUNT;

  save_rb_state(logger);

  return status;
}

HAL_StatusTypeDef logger_log_tail_page(LoggerHandleDef *logger, LogPage *tail) {
  HAL_GUARD(logger->eeprom_is_ready);

  *tail = TAIL_PAGE(logger->head_page);

  return logger->eeprom_is_ready;
}
