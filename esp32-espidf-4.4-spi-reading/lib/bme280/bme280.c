#include "bme280.h"

#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// bit 7 of command byte 1 = read, 0 = write
#define BME280_READ_BIT 0x80U 
// command byte + payload
#define BME280_FRAME_MAX 32U 

// dig_T1..dig_P9, 0xA0 unused, 0xA1 = dig_H1
#define BME280_REG_CALIB_TP 0x88U
#define BME280_REG_CALIB_TP_LEN 26U

// dig_H2..dig_H6
#define BME280_REG_CLIB_H 0xE1U 
#define BME280_CLIB_H_LEN 7U

#define BME280_REG_CTRL_HUM  0xF2U
#define BME280_REG_STATUS    0xF3U
#define BME280_REG_CTRL_MEAS 0xF4U

// press[3], temp[3], hum[2]
#define BME280_REG_DATA 0xF7U
#define BME280_DATA_LEN 8U

#define BME280_START_MEASURING 0x08U
// osrs_h = x1
#define BME280_CTRL_HUM_X1 0x01U

// osrs_t = x1, osrs_p = 1, mode = forced
#define BME280_MEAS_FORCED 0x25U

#define BME280_REG_ID 0xD0U
#define BME280_MEASURE_POLL_MS 10U
#define BME280_MEASURE_POLLS 5U

static const char *TAG = "bme280";

struct bme280_calib_s {
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;

    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;

    uint8_t dig_H1;
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;
};

struct bme280_s {
    spi_device_handle_t spi;
    struct bme280_calib_s calib;
};

static uint16_t u16_le(const uint8_t *d)
{
    return (uint16_t)(d[0] | ((uint16_t)d[1] << 8));
}

static int16_t s16_le(const uint8_t *d)
{
    return (int16_t)u16_le(d);
}

esp_err_t bme280_read_registers(bme280_handle_t bme, uint8_t reg, uint8_t *data, size_t len)
{
    if (bme == NULL || data == NULL || len == 0U || len + 1U > BME280_FRAME_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t tx[BME280_FRAME_MAX] = {0};
    uint8_t rx[BME280_FRAME_MAX] = {0};
    tx[0] = reg | BME280_READ_BIT;

    // NOTE: the first register value arrives during the second byte,
    // so the payload is rx[1..len]
    spi_transaction_t t = {0};
    t.length = (len + 1U) * 8U;
    t.tx_buffer = tx;
    t.rx_buffer = rx;

    const esp_err_t err = spi_device_polling_transmit(bme->spi, &t);
    if (err != ESP_OK) {
        return err;
    }

    memcpy(data, &rx[1], len);
    return ESP_OK;
}

esp_err_t bme280_write_register(bme280_handle_t bme, uint8_t reg, uint8_t value)
{
    if (bme == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const uint8_t tx[2] = {(uint8_t)(reg & ~BME280_READ_BIT), value};
    spi_transaction_t t = {0};
    t.length = sizeof(tx) * 8U;
    t.tx_buffer = tx;

    return spi_device_polling_transmit(bme->spi, &t);
}

static esp_err_t read_calibration(bme280_handle_t bme)
{
    uint8_t tp[BME280_REG_CALIB_TP_LEN];
    uint8_t h[BME280_CLIB_H_LEN];

    esp_err_t err = bme280_read_registers(bme, BME280_REG_CALIB_TP, tp, sizeof(tp));
    if (err != ESP_OK) {
        return err;
    }
    err = bme280_read_registers(bme, BME280_REG_CLIB_H, h, sizeof(h));
    if (err != ESP_OK) {
        return err;
    }

    struct bme280_calib_s *c = &bme->calib;
    c->dig_T1 = u16_le(&tp[0]);
    c->dig_T2 = s16_le(&tp[2]);
    c->dig_T3 = s16_le(&tp[4]);

    c->dig_P1 = u16_le(&tp[6]);
    c->dig_P2 = s16_le(&tp[8]);
    c->dig_P3 = s16_le(&tp[10]);
    c->dig_P4 = s16_le(&tp[12]);
    c->dig_P5 = s16_le(&tp[14]);
    c->dig_P6 = s16_le(&tp[16]);
    c->dig_P7 = s16_le(&tp[18]);
    c->dig_P8 = s16_le(&tp[20]);
    c->dig_P9 = s16_le(&tp[22]);

    c->dig_H1 = tp[25];
    c->dig_H2 = s16_le(&h[0]);
    c->dig_H3 = h[2];
    // NOTE: H4/H5 are signed 12-bit values sharing the nibbles of 0xE5
    c->dig_H4 = (int16_t)(((int16_t)(int8_t)h[3] * 16) | (h[4] & 0x0FU));
    c->dig_H5 = (int16_t)(((int16_t)(int8_t)h[5] * 16) | (h[4] >> 4));
    c->dig_H6 = (int8_t)h[6];

    ESP_LOGI(TAG, "calib T: %u %d %d", c->dig_T1, c->dig_T2, c->dig_T3);
    ESP_LOGI(TAG, "calib P: %u %d %d %d %d %d %d %d %d", c->dig_P1, c->dig_P2,
             c->dig_P3, c->dig_P4, c->dig_P5, c->dig_P6, c->dig_P7, c->dig_P8,
             c->dig_P9);
    ESP_LOGI(TAG, "calib H: %u %d %u %d %d %d", c->dig_H1, c->dig_H2, c->dig_H3,
             c->dig_H4, c->dig_H5, c->dig_H6);
    return ESP_OK;
}

/* NOTE: compensation formulas are Bosch reference code from the BME280 datasheet */

// Returns t_fine, shared by the pressure and humidity formulas.
static int32_t compensate_t_fine(int32_t adc_T, const struct bme280_calib_s *c)
{
    const int32_t var1 =
        ((((adc_T >> 3) - ((int32_t)c->dig_T1 << 1))) * ((int32_t)c->dig_T2)) >> 11;
    const int32_t var2 = (((((adc_T >> 4) - ((int32_t)c->dig_T1)) *
                           ((adc_T >> 4) - ((int32_t)c->dig_T1))) >> 12) *
                          ((int32_t)c->dig_T3)) >> 14;
    return var1 + var2;
}

static float compensate_pressure_hpa(int32_t adc_P, int32_t t_fine, const struct bme280_calib_s *c)
{
    int64_t var1 = ((int64_t)t_fine) - 128000;
    int64_t var2 = var1 * var1 * (int64_t)c->dig_P6;
    var2 = var2 + ((var1 * (int64_t)c->dig_P5) << 17);
    var2 = var2 + (((int64_t)c->dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)c->dig_P3) >> 8) + ((var1 * (int64_t)c->dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1) * ((int64_t)c->dig_P1)) >> 33;

    if (var1 == 0) {
        return 0.0f; // avoid division by zero
    }

    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)c->dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)c->dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)c->dig_P7) << 4);

    // NOTE: p is Pa in Q24.8 fixed point
    return ((float)p / 256.0f) / 100.0f;
}

static float compensate_humidity_rh(int32_t adc_H, int32_t t_fine, const struct bme280_calib_s *c)
{
    float h = ((float)t_fine) - 76800.0f;
    h = ((float)adc_H - (((float)c->dig_H4) * 64.0f + ((float)c->dig_H5) / 16384.0f * h)) *
        (((float)c->dig_H2) / 65536.0f *
         (1.0f + ((float)c->dig_H6) / 67108864.0f * h *
                     (1.0f + ((float)c->dig_H3) / 67108864.0f * h)));
    h = h * (1.0f - ((float)c->dig_H1) * h / 524288.0f);

    if (h > 100.0f) {
        return 100.0f;
    }
    if (h < 0.0f) {
        return 0.0f;
    }
    return h;
}

esp_err_t bme280_init(const bme280_settings_t *settings, bme280_handle_t *bme)
{
    if (settings == NULL || bme == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *bme = NULL;

    struct bme280_s *ctx = calloc(1U, sizeof(*ctx));
    if (ctx == NULL) {
        return ESP_ERR_NO_MEM;
    }

    const spi_device_interface_config_t dev_cfg = {
        .mode = 0, // CPOL=0, CPHA=0 - matches STM32 POLARITY_LOW / PHASE_1EDGE
        .clock_speed_hz = settings->clock_speed_hz,
        .spics_io_num = settings->cs_pin,
        .queue_size = 1,
    };

    esp_err_t err = spi_bus_add_device(settings->host, &dev_cfg, &ctx->spi);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SPI device add failed: %s", esp_err_to_name(err));
        free(ctx);
        return err;
    }

    uint8_t chip_id = 0;
    err = bme280_read_registers(ctx, BME280_REG_ID, &chip_id, 1U);
    if (err == ESP_OK && chip_id != BME280_CHIP_ID) {
        ESP_LOGE(TAG, "unexpected chip id 0x%02X (want 0x%02X)", chip_id, BME280_CHIP_ID);
        err = ESP_ERR_NOT_FOUND;
    }
    if (err == ESP_OK) {
        err = read_calibration(ctx);
    }
    if (err != ESP_OK) {
        spi_bus_remove_device(ctx->spi);
        free(ctx);
        return err;
    }

    *bme = ctx;
    return ESP_OK;
}

esp_err_t bme280_deinit(bme280_handle_t bme)
{
    if (bme == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_err_t err = spi_bus_remove_device(bme->spi);
    if (err != ESP_OK) {
        return err;
    }
    free(bme);
    return ESP_OK;
}

esp_err_t bme280_measure(bme280_handle_t bme, bme280_measurement_t *out)
{
    if (bme == NULL || out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // NOTE: ctrl_hum only takes effect after the following ctrl_meas write
    esp_err_t err = bme280_write_register(bme, BME280_REG_CTRL_HUM, BME280_CTRL_HUM_X1);
    if (err != ESP_OK) {
        return err;
    }
    err = bme280_write_register(bme, BME280_REG_CTRL_MEAS, BME280_MEAS_FORCED);
    if (err != ESP_OK) {
        return err;
    }

    // NOTE: x1/x1/x1 takes up to ~10 ms; poll status.measuring until it clears
    uint8_t status = BME280_START_MEASURING;
    for (uint8_t i = 0; i < BME280_MEASURE_POLLS && (status & BME280_START_MEASURING) != 0U;
         i++) {
        vTaskDelay(pdMS_TO_TICKS(BME280_MEASURE_POLL_MS));
        err = bme280_read_registers(bme, BME280_REG_STATUS, &status, 1U);
        if (err != ESP_OK) {
            return err;
        }
    }
    if ((status & BME280_START_MEASURING) != 0U) {
        return ESP_ERR_TIMEOUT;
    }

    // NOTE: one burst read keeps press/temp/hum from the same measurement
    uint8_t d[BME280_DATA_LEN];
    err = bme280_read_registers(bme, BME280_REG_DATA, d, sizeof(d));
    if (err != ESP_OK) {
        return err;
    }

    const int32_t adc_P = ((int32_t)d[0] << 12) | ((int32_t)d[1] << 4) | (d[2] >> 4);
    const int32_t adc_T = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | (d[5] >> 4);
    const int32_t adc_H = ((int32_t)d[6] << 8) | d[7];

    const int32_t t_fine = compensate_t_fine(adc_T, &bme->calib);
    out->temperature_c = (float)((t_fine * 5 + 128) >> 8) / 100.0f;
    out->pressure_hpa = compensate_pressure_hpa(adc_P, t_fine, &bme->calib);
    out->humidity_rh = compensate_humidity_rh(adc_H, t_fine, &bme->calib);
    return ESP_OK;
}
