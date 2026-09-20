
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_adc/adc_oneshot.h"
#include "soc/adc_channel.h"
// #include "esp_check.h"
#include "esp_adc/adc_cali.h"
#include "esp_err.h"
#include <math.h>

#define POT_CHANNEL ADC_CHANNEL_3 // GPIO4

// Manual conversion assumes a perfect Vref at full-scale.
// For ADC_ATTEN_DB_12 on a 12-bit ADC the nominal full-scale is ~3300 mV.
#define ADC_MAX_RAW 4095
#define VREF_MV 3300

extern "C" void app_main() {
  // Initializing ADC1 Unit
  adc_oneshot_unit_handle_t adc1_handle;
  adc_oneshot_unit_init_cfg_t adc1_config = {.unit_id = ADC_UNIT_1,
                                             .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
                                             .ulp_mode = ADC_ULP_MODE_DISABLE};
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc1_config, &adc1_handle));

  // Initializing ADC CHANNEL for the potentiometer
  adc_oneshot_chan_cfg_t adc_chan_cfg = {.atten = ADC_ATTEN_DB_12,
                                         .bitwidth = ADC_BITWIDTH_DEFAULT};
  ESP_ERROR_CHECK(
      adc_oneshot_config_channel(adc1_handle, POT_CHANNEL, &adc_chan_cfg));

  adc_cali_handle_t cali_handle;
  adc_cali_curve_fitting_config_t cali_config = {.unit_id = adc1_config.unit_id,
                                                 .chan = POT_CHANNEL,
                                                 .atten = adc_chan_cfg.atten,
                                                 .bitwidth =
                                                     adc_chan_cfg.bitwidth};

  // Initializing calibration scheme for the potentiometer ADC CHANNEL
  ESP_ERROR_CHECK(
      adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle));

  // Table header
  printf("%6s | %12s | %10s | %9s\n", "RAW", "U_manual(mV)", "U_cali(mV)",
         "Error(%)");

  while (1) {
    int raw = 0;
    int u_cali = 0;

    // Read raw ADC value
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, POT_CHANNEL, &raw));

    // Manual voltage assuming a perfect Vref
    int u_manual = (int)((int64_t)raw * VREF_MV / ADC_MAX_RAW);

    // Calibrated voltage
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw, &u_cali));

    float error = 0;
    if (u_cali != 0) {
      error = ((float)(u_manual - u_cali) / (float)u_cali) * 100.0f;
    }

    printf("%6d | %12d | %10d | %+9.2f\n", raw, u_manual, u_cali, error);

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}
