#ifndef BME280_DRIVER_H
#define BME280_DRIVER_H

#include <stddef.h>
#include <stdint.h>

#include "driver/spi_master.h"
#include "esp_err.h"
#include "soc/gpio_num.h"

#define BME280_CHIP_ID 0x60U

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque BME280 handle. */
typedef struct bme280_s *bme280_handle_t;

/* Device settings; the SPI bus must already be initialized. */
typedef struct {
    spi_host_device_t host;
    gpio_num_t cs_pin;
    int clock_speed_hz;
} bme280_settings_t;

/* Compensated measurement. */
typedef struct {
    float temperature_c;
    float pressure_hpa;
    float humidity_rh;
} bme280_measurement_t;

/*
 * Adds BME280 (SPI mode 0) to the bus, checks the chip ID and reads calibration.
 * Returns ESP_ERR_NOT_FOUND when the chip ID is not 0x60.
 */
esp_err_t bme280_init(const bme280_settings_t *settings, bme280_handle_t *bme);

/* Removes the device; the SPI bus stays initialized. */
esp_err_t bme280_deinit(bme280_handle_t bme);

/* Reads `len` registers starting at `reg` (auto-increment). */
esp_err_t bme280_read_registers(bme280_handle_t bme, uint8_t reg, uint8_t *data, size_t len);

/* Writes one register. */
esp_err_t bme280_write_register(bme280_handle_t bme, uint8_t reg, uint8_t value);

/* Runs one forced-mode measurement (x1 oversampling) and compensates it. */
esp_err_t bme280_measure(bme280_handle_t bme, bme280_measurement_t *out);

#ifdef __cplusplus
}
#endif

#endif /* BME280_DRIVER_H */
