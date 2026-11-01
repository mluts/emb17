#include "logger.h"
#include "at24c32.h"
#include "i2c.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_i2c.h"
#include "usbd_def.h"
#include <stdint.h>

#define LOG_STATUS_MAGIC 0xABAD1DEA

typedef struct {
  LogPage head_page;
  uint32_t magic; // should be equal to LOG_STATUS_MAGIC
} LogStatusDef;

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif /* MIN */

#define HAL_GUARD(status)                                                      \
  if (status != HAL_OK) {                                                      \
    return status;                                                             \
  }

/// @brief Save logging state to EEPROM reserved page
static HAL_StatusTypeDef save_logger_state(LoggerHandleDef *logger) {
  LogStatusDef log_status = {0};

  HAL_GUARD(logger->eeprom_is_ready);

  log_status.head_page = logger->head_page;
  log_status.magic = LOG_STATUS_MAGIC;

  return AT24C32_Write(logger->eeprom, LOG_STATUS_PAGE_ADDR,
                       (uint8_t *)&log_status, sizeof(log_status));
}

/// @brief Read logging state from EEPROM reserved page
static HAL_StatusTypeDef restore_logger_state(LoggerHandleDef *logger) {
  LogStatusDef log_status = {0};
  HAL_StatusTypeDef status = 0;

  HAL_GUARD(logger->eeprom_is_ready);

  status = AT24C32_Read(logger->eeprom, LOG_STATUS_PAGE_ADDR,
                        (uint8_t *)&log_status, sizeof(log_status));

  HAL_GUARD(status);

  if (log_status.magic == LOG_STATUS_MAGIC) {
    logger->head_page = log_status.head_page;
  } else {
    logger->head_page = 0;
  }

  return status;
}

HAL_StatusTypeDef logger_init(LoggerHandleDef *logger,
                              AT24C32_HandleTypeDef *eeprom) {

  logger->eeprom = eeprom;
  logger->head_page = 0;

  HAL_GUARD(logger->eeprom_is_ready);

  restore_logger_state(logger);

  return logger->eeprom_is_ready;
}

/// @brief read log page from logs
/// @param logger: logger handle
/// @param log_number: log page
/// @param log: pointer which will contain log, extracted from memory
/// @retval HAL Status of peripheral operation
HAL_StatusTypeDef logger_read(const LoggerHandleDef *logger, LogPage log_number,
                              LogDef *log) {
  HAL_GUARD(logger->eeprom_is_ready);

  return AT24C32_Read(logger->eeprom, LOG_PAGE_ADDR(log_number), (uint8_t *)log,
                      sizeof(LogDef));
}

/// @brief Add new log
/// @param logger: logger handle
/// @param msg: string to be appended as log
/// @retval HAL Status of peripheral operation
HAL_StatusTypeDef logger_write(LoggerHandleDef *logger, const char *msg) {
  HAL_GUARD(logger->eeprom_is_ready);

  static HAL_StatusTypeDef status;
  LogDef log = {
      .log_page = logger->head_page % LOG_MAX_COUNT,
  };

  strncpy((char *)log.log, msg, sizeof(log.log) - 2);
  log.log[sizeof(log.log) - 1] = '\0';

  status = AT24C32_Write(logger->eeprom, LOG_PAGE_ADDR(logger->head_page),
                         (uint8_t *)&log, sizeof(LogDef));

  HAL_GUARD(status);

  logger->head_page++;

  save_logger_state(logger);

  return status;
}

/// @brief Return most early page. Logger data is stored in ring buffer with
/// total EEPROM_PAGE_COUNT pages
/// @param logger: logger handle
/// @param tail: pointer to tail
/// @retval HAL Status of peripheral operation
HAL_StatusTypeDef logger_log_tail(const LoggerHandleDef *logger,
                                  LogPage *tail) {
  HAL_GUARD(logger->eeprom_is_ready);

  *tail = TAIL_PAGE(logger->head_page);

  return logger->eeprom_is_ready;
}
