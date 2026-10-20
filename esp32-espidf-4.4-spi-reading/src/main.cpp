#include <stdio.h>
#include <string.h>

#include "driver/spi_common.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/task.h"

#include "esp_err.h"
#include "driver/spi_master.h"
#include "soc/gpio_num.h"

// NOTE: SPI2 on IOMUX
#define SPI_HOST_ID   SPI2_HOST
#define SPI_PIN_MOSI  GPIO_NUM_11
#define SPI_PIN_SCLK  GPIO_NUM_12
#define SPI_PIN_MISO  GPIO_NUM_13
#define SPI_PIN_CS    GPIO_NUM_10

// NOTE: STM32 slave re-arms HAL_SPI_TransmitReceive_IT per byte, so keep clock low
#define SPI_CLOCK_HZ (100 * 1000)

// NOTE: BME280 register map, emulated by the STM32 on SPI2
#define BME280_REG_ID    0xD0
#define BME280_READ      0x80 // bit 7 of the command byte selects read
#define BME280_FRAME_MAX 32

static spi_device_handle_t spi_dev;

void spi_init() {
  spi_bus_config_t bus_cfg = {};
  bus_cfg.mosi_io_num = SPI_PIN_MOSI;
  bus_cfg.miso_io_num = SPI_PIN_MISO;
  bus_cfg.sclk_io_num = SPI_PIN_SCLK;
  bus_cfg.quadwp_io_num = -1;
  bus_cfg.quadhd_io_num = -1;
  bus_cfg.max_transfer_sz = 32;

  ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST_ID, &bus_cfg, SPI_DMA_DISABLED));

  spi_device_interface_config_t dev_cfg = {};
  dev_cfg.mode = 0; // CPOL=0, CPHA=0 - matches STM32 POLARITY_LOW / PHASE_1EDGE
  dev_cfg.clock_speed_hz = SPI_CLOCK_HZ;
  dev_cfg.spics_io_num = SPI_PIN_CS;
  dev_cfg.cs_ena_posttrans = 2;
  dev_cfg.queue_size = 1;

  ESP_ERROR_CHECK(spi_bus_add_device(SPI_HOST_ID, &dev_cfg, &spi_dev));

  printf("[SPI] master ready: SCLK=%d MOSI=%d MISO=%d CS=%d @ %d Hz\n",
         SPI_PIN_SCLK, SPI_PIN_MOSI, SPI_PIN_MISO, SPI_PIN_CS, SPI_CLOCK_HZ);
}

// NOTE: command byte + (len) clock bytes; the first register value arrives
// during the second byte, so the payload is rx[1..len]
static esp_err_t bme280_read_regs(uint8_t reg, uint8_t *out, size_t len) {
  if (out == nullptr || len == 0 || len + 1 > BME280_FRAME_MAX) {
    return ESP_ERR_INVALID_ARG;
  }

  uint8_t tx[BME280_FRAME_MAX] = {};
  uint8_t rx[BME280_FRAME_MAX] = {};
  tx[0] = reg | BME280_READ;

  spi_transaction_t t = {};
  t.length = (len + 1) * 8;
  t.tx_buffer = tx;
  t.rx_buffer = rx;

  esp_err_t err = spi_device_polling_transmit(spi_dev, &t);
  if (err != ESP_OK) {
    return err;
  }

  memcpy(out, rx + 1, len);
  return ESP_OK;
}

extern "C" void app_main() {
  spi_init();

  while (1) {
    uint8_t chip_id = 0;
    esp_err_t err = bme280_read_regs(BME280_REG_ID, &chip_id, 1);
    if (err == ESP_OK) {
      printf("[BME280] chip id = 0x%02X\n", chip_id);
    } else {
      printf("[BME280] read failed: %s\n", esp_err_to_name(err));
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
