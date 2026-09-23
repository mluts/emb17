
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "soc/adc_channel.h"
// #include "esp_check.h"
#include "esp_adc/adc_cali.h"
#include "esp_err.h"

#define POT_CHANNEL ADC_CHANNEL_3 // GPIO4

// For ADC_ATTEN_DB_12 on a 12-bit ADC the nominal full-scale is ~3300 mV.
#define VREF_MV 3300

// LEDC
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_RESOLUTION LEDC_TIMER_10_BIT
#define LEDC_FREQ_HZ 1000

#define LEDC_CHANNEL_0_GPIO 5
#define LEDC_CHANNEL_1_GPIO 6

#define LEDC_MAX_DUTY ((1 << LEDC_RESOLUTION) - 1)

uint32_t mv_to_duty(int mv) {
  if (mv <= 0) {
    return 0;
  }
  if (mv >= VREF_MV) {
    return LEDC_MAX_DUTY;
  }

  return (uint32_t)mv * LEDC_MAX_DUTY / VREF_MV;
}

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

  // Initializing LEDC timer
  ledc_timer_config_t ledc_timer = {.speed_mode = LEDC_MODE,
                                    .duty_resolution = LEDC_RESOLUTION,
                                    .timer_num = LEDC_TIMER,
                                    .freq_hz = LEDC_FREQ_HZ,
                                    .clk_cfg = LEDC_AUTO_CLK,
                                    .deconfigure = false};
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  ledc_channel_config_t ledc_ch0 = {
      .gpio_num = LEDC_CHANNEL_0_GPIO,
      .speed_mode = LEDC_MODE,
      .channel = LEDC_CHANNEL_0,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = LEDC_TIMER,
      .duty = 0,
      .hpoint = 0,
      .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
      .flags = {.output_invert = 0},
      .deconfigure = false,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_ch0));

  ledc_channel_config_t ledc_ch1 = {
      .gpio_num = LEDC_CHANNEL_1_GPIO,
      .speed_mode = LEDC_MODE,
      .channel = LEDC_CHANNEL_1,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = LEDC_TIMER,
      .duty = 0,
      .hpoint = 0,
      .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
      .flags = {.output_invert = 0},
      .deconfigure = false,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_ch1));

  while (1) {
    int raw = 0;
    int u_cali = 0;

    // Read raw ADC value
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, POT_CHANNEL, &raw));

    // Calibrated voltage
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw, &u_cali));

    ESP_ERROR_CHECK(
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, mv_to_duty(u_cali)));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
    ESP_ERROR_CHECK(
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, mv_to_duty(u_cali)));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));

    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}
