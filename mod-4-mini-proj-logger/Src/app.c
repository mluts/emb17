#include "at24c32.h"
#include "i2c.h"
#include "i2c_ext.h"
#include "logger.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_i2c.h"
#include "usb_printf.h"
#include <console.h>
#include <ctype.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define BLINK_PERIOD_MS 500
#define PROMPT "> "
#define AT24C32_A_PINS 0

static LoggerHandleDef logger = {};
static AT24C32_HandleTypeDef eeprom;

// Shows found i2c devices
void i2c_scan() {

  HAL_StatusTypeDef status;

  /* Сканування шини I2C */
  uint8_t found[16];
  uint8_t found_count = 0;

  status = I2C_Scan(&hi2c1, found, sizeof(found), &found_count, 10);
  if (status != HAL_OK) {
    printf("[I2C] Scan failed (status=%d)\r\n", (int)status);
    return;
  }

  printf("[I2C] Scan: %u device(s) found\r\n", (unsigned)found_count);
  for (uint8_t i = 0; (i < found_count) && (i < sizeof(found)); i++) {
    printf("[I2C] Found device at 0x%02X\r\n", found[i]);
  }
}

// Simple fn for verifying that console works
void pong(const char *s) { printf("\r\npong\r\n"); }

// Blink every BLINK_PERIOD_MS
void blink() {
  static uint32_t last_blink = 0;

  if (HAL_GetTick() - last_blink > BLINK_PERIOD_MS) {

    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    last_blink = HAL_GetTick();
  }
}

const char *hal_err_to_str(HAL_StatusTypeDef err) {
  switch (err) {
  case HAL_OK:
    return "HAL_OK";
    break;
  case HAL_ERROR:
    return "HAL_ERROR";
    break;
  case HAL_BUSY:
    return "HAL_BUSY";
    break;
  case HAL_TIMEOUT:
    return "HAL_TIMEOUT";
    break;
  default:
    return "";
  }
}

void eeprom_init() {
  static HAL_StatusTypeDef status;

  printf("\r\nInitializing EEPROM AT24C32(A%u)\r\n", AT24C32_A_PINS);

  status = AT24C32_Init(&eeprom, &hi2c1, AT24C32_A_PINS);

  if (status != HAL_OK) {
    printf("\r\nEEPROM AT24C32(A%u) Init Error: %s\r\n", AT24C32_A_PINS,
           hal_err_to_str(status));
  }

  printf("\r\nEEPROM AT24C32(A%u) Connected\r\n", AT24C32_A_PINS);
}

void eeprom_is_ready() {
  printf("\r\nEEPROM AT24C32(A%u) readiness: %s\r\n", AT24C32_A_PINS,
         hal_err_to_str(AT24C32_IsReady(&eeprom)));
}

void do_logger_init() {
  HAL_StatusTypeDef status = 0;

  printf("\r\nInitializing logger!\r\n");
  status = logger_init(&logger, &eeprom);

  if (status == HAL_OK) {
    printf("\r\nLogger initialized!\r\n");
  } else {
    printf("\r\nLogger initialization error: %u\r\n", status);
  }
}

void logger_log(const char *cmd) {
  while (*cmd != '\0' && !isspace(*cmd)) {
    cmd++;
  }

  while (*cmd != '\0' && isspace(*cmd)) {
    cmd++;
  }

  if (logger_write(&logger, cmd) == HAL_OK) {
    printf("\r\nSaved log: %s\r\n", cmd);
  } else {
    printf("\r\nlogger write failed!\r\n");
    return;
  }
}

void do_logger_read() {
  LogPage tail = 0;
  LogDef log = {0};

  if (logger_log_tail_page(&logger, &tail) != HAL_OK) {
    printf("\r\nlogger error\r\n");
    return;
  }

  printf("\r\nLogs tail: %u\r\n", tail);
  printf("Logs head page: %u\r\n", logger.head_page);

  for (LogPage i = tail; i < logger.head_page; i++) {
    logger_read(&logger, i, &log);
    printf("(%u) %s\r\n", log.log_page, log.log);
  }
}

void app_setup() {
  console_add_command("ping", pong);
  console_add_command("i2c-scan", i2c_scan);
  console_add_command("eeprom-init", eeprom_init);
  console_add_command("eeprom-is-ready", eeprom_is_ready);
  console_add_command("logger-init", do_logger_init);
  console_add_command("log", logger_log);
  console_add_command("logs-read", do_logger_read);

  eeprom_init();
  do_logger_init();
}

void app_main() {

  while (1) {
    blink();
    console_read_line();
  }
}
