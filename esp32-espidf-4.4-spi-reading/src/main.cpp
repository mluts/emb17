#include <stdio.h>

#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/task.h"

#include "esp_err.h"
#include "soc/gpio_num.h"

#include "bme280.h"
#include "ds1307.h"
#include "i2c.h"
#include "oled.h"

// NOTE: SPI2 on IOMUX
#define SPI_HOST_ID   SPI2_HOST
#define SPI_PIN_MOSI  GPIO_NUM_11
#define SPI_PIN_SCLK  GPIO_NUM_12
#define SPI_PIN_MISO  GPIO_NUM_13
#define SPI_PIN_CS    GPIO_NUM_10

// NOTE: STM32 slave re-arms HAL_SPI_TransmitReceive_IT per byte, so keep clock low
#define SPI_CLOCK_HZ (100 * 1000)

// NOTE: I2C0 is shared by SSD1306 (0x3C, 400 kHz) and DS1307 (0x68, 100 kHz)
#define I2C_PORT     I2C_NUM_0
#define I2C_PIN_SDA  GPIO_NUM_8
#define I2C_PIN_SCL  GPIO_NUM_9

#define UPDATE_PERIOD_MS 1000

// NOTE: written only when the DS1307 oscillator is halted (no time kept on battery)
static const ds1307_datetime_t initial_datetime = {.year = 2026,
                                                   .month = 10,
                                                   .day = 20,
                                                   .weekday = 3, // Tuesday
                                                   .hour = 21,
                                                   .minute = 0,
                                                   .second = 0};

// NOTE: don't use in `if () RETURN_IF_ERR;`
#define RETURN_IF_ERR(err, tag, msg)                                           \
  if ((err) != ESP_OK) {                                                       \
    printf("[" tag "] " msg ": %s\n", esp_err_to_name(err));                   \
    return;                                                                    \
  }

// NOTE: DS1307 weekday is 1 = Sunday ... 7 = Saturday
static const char *weekday_name(uint8_t weekday) {
  static const char *const names[] = {"Sun", "Mon", "Tue", "Wed",
                                      "Thu", "Fri", "Sat"};
  if ((weekday < 1) || (weekday > 7)) {
    return "???";
  }
  return names[weekday - 1];
}

void spi_init() {
  spi_bus_config_t bus_cfg = {};
  bus_cfg.mosi_io_num = SPI_PIN_MOSI;
  bus_cfg.miso_io_num = SPI_PIN_MISO;
  bus_cfg.sclk_io_num = SPI_PIN_SCLK;
  bus_cfg.quadwp_io_num = -1;
  bus_cfg.quadhd_io_num = -1;
  bus_cfg.max_transfer_sz = 32;

  ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST_ID, &bus_cfg, SPI_DMA_DISABLED));

  printf("[SPI] bus ready: SCLK=%d MOSI=%d MISO=%d\n",
         SPI_PIN_SCLK, SPI_PIN_MOSI, SPI_PIN_MISO);
}

extern "C" void app_main() {
  printf("Starting time and BME280 display on SSD1306\n");

  spi_init();

  i2c_bus_settings_t i2c_cfg = {};
  i2c_cfg.port = I2C_PORT;
  i2c_cfg.sda_pin = I2C_PIN_SDA;
  i2c_cfg.scl_pin = I2C_PIN_SCL;
  i2c_cfg.enable_internal_pullup = true;
  i2c_master_bus_handle_t i2c_bus = nullptr;
  esp_err_t err = i2c_bus_init(&i2c_cfg, &i2c_bus);
  RETURN_IF_ERR(err, "I2C", "bus init failed");

  oled_settings_t oled_cfg = OLED_SETTINGS_128X64_DEFAULT();
  oled_handle_t oled = nullptr;
  err = oled_init(i2c_bus, &oled_cfg, &oled);
  RETURN_IF_ERR(err, "OLED", "init failed");

  ds1307_handle_t rtc = nullptr;
  err = ds1307_init(i2c_bus, &rtc);
  RETURN_IF_ERR(err, "RTC", "init failed");

  // NOTE: don't override the time kept on battery
  bool running = false;
  err = ds1307_is_running(rtc, &running);
  RETURN_IF_ERR(err, "RTC", "oscillator check failed");
  if (!running) {
    err = ds1307_set_datetime(rtc, &initial_datetime);
    RETURN_IF_ERR(err, "RTC", "set datetime failed");
  }

  bme280_settings_t bme_cfg = {};
  bme_cfg.host = SPI_HOST_ID;
  bme_cfg.cs_pin = SPI_PIN_CS;
  bme_cfg.clock_speed_hz = SPI_CLOCK_HZ;
  bme280_handle_t bme = nullptr;
  err = bme280_init(&bme_cfg, &bme);
  RETURN_IF_ERR(err, "BME280", "init failed");

  err = oled_clear(oled);
  RETURN_IF_ERR(err, "OLED", "clear failed");
  err = oled_print(oled, 0, 1, false, "DS1307 + BME280");
  RETURN_IF_ERR(err, "OLED", "draw header failed");

  // NOTE: oled_print logs its own failures, so the loop doesn't check them
  while (1) {
    ds1307_datetime_t dt = {};
    err = ds1307_get_datetime(rtc, &dt);
    if (err == ESP_OK) {
      oled_printf(oled, 1, 1, "%s %02u.%02u.%04u", weekday_name(dt.weekday),
                  (unsigned)dt.day, (unsigned)dt.month, (unsigned)dt.year);
      oled_printf(oled, 2, 2, "%02u:%02u:%02u", (unsigned)dt.hour,
                  (unsigned)dt.minute, (unsigned)dt.second);
    } else {
      printf("[RTC] read failed: %s\n", esp_err_to_name(err));
    }

    bme280_measurement_t m = {};
    err = bme280_measure(bme, &m);
    if (err == ESP_OK) {
      oled_printf(oled, 5, 1, "T: %7.2f C", m.temperature_c);
      oled_printf(oled, 6, 1, "H: %7.2f %%", m.humidity_rh);
      oled_printf(oled, 7, 1, "P: %7.2f hPa", m.pressure_hpa);
    } else {
      printf("[BME280] measure failed: %s\n", esp_err_to_name(err));
    }

    vTaskDelay(pdMS_TO_TICKS(UPDATE_PERIOD_MS));
  }
}
