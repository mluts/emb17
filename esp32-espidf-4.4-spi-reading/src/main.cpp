#include <stdio.h>
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
  dev_cfg.cs_ena_pretrans = 2; // give STM32 EXTI on NSS time to reset its state
  dev_cfg.cs_ena_posttrans = 2;
  dev_cfg.queue_size = 1;

  ESP_ERROR_CHECK(spi_bus_add_device(SPI_HOST_ID, &dev_cfg, &spi_dev));

  printf("[SPI] master ready: SCLK=%d MOSI=%d MISO=%d CS=%d @ %d Hz\n",
         SPI_PIN_SCLK, SPI_PIN_MOSI, SPI_PIN_MISO, SPI_PIN_CS, SPI_CLOCK_HZ);
}

extern "C" void app_main() {
  spi_init();

  while (1) {
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}
