
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

#define LDR_CHANNEL ADC_CHANNEL_3 // GPIO4

#define LDR_THRESHOLD 1000
#define LDR_DELTA 200
#define LDR_SAMPLES 100

#define LED_GPIO GPIO_NUM_5

// Reads calibrated voltage
int read_raw_voltage(adc_oneshot_unit_handle_t unit_handle, adc_channel_t chan,
                     adc_cali_handle_t cali_handle) {
  int adc_raw_val, adc_mv;
  // Read raw val
  ESP_ERROR_CHECK(adc_oneshot_read(unit_handle, chan, &adc_raw_val));
  // raw -> calibrated voltage
  ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, adc_raw_val, &adc_mv));

  return adc_mv;
}

// Reads raw voltage <samples> times AND returns Simple Moving Average
int read_sma_voltage(adc_oneshot_unit_handle_t unit_handle, adc_channel_t chan,
                     adc_cali_handle_t cali_handle, int samples) {
  int sum = 0;
  if (samples <= 0) {
    return -1;
  }

  for (int i = 0; i < samples; i++) {
    sum += read_raw_voltage(unit_handle, chan, cali_handle);
  }

  return sum / samples;
}

extern "C" void app_main() {
  // Initializing ADC1 Unit
  adc_oneshot_unit_handle_t adc1_handle;
  adc_oneshot_unit_init_cfg_t adc1_config = {.unit_id = ADC_UNIT_1,
                                             .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
                                             .ulp_mode = ADC_ULP_MODE_DISABLE};
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc1_config, &adc1_handle));

  // Initializing ADC CHANNEL for LDR
  adc_oneshot_chan_cfg_t adc_chan_cfg = {.atten = ADC_ATTEN_DB_12,
                                         .bitwidth = ADC_BITWIDTH_DEFAULT};
  ESP_ERROR_CHECK(
      adc_oneshot_config_channel(adc1_handle, LDR_CHANNEL, &adc_chan_cfg));

  adc_cali_handle_t cali_handle;
  adc_cali_curve_fitting_config_t cali_config = {.unit_id = adc1_config.unit_id,
                                                 .chan = LDR_CHANNEL,
                                                 .atten = adc_chan_cfg.atten,
                                                 .bitwidth =
                                                     adc_chan_cfg.bitwidth};

  // Initializing calibration scheme for LDR ADC CHANNEL
  ESP_ERROR_CHECK(
      adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle));

  // Initialize LED GPIO
  gpio_config_t gpio_led_conf = {};
  gpio_led_conf.pin_bit_mask = 1ULL << LED_GPIO;
  gpio_led_conf.mode = GPIO_MODE_OUTPUT;
  // (good practice)
  // Disabling pullup/pulldown for OUTPUT
  gpio_led_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_led_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  // disable interrupts
  gpio_led_conf.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&gpio_led_conf);

  while (1) {
    static bool light = false;
    static int ldr_mv = 0;

    // Read LDR Voltage
    ldr_mv =
        read_sma_voltage(adc1_handle, LDR_CHANNEL, cali_handle, LDR_SAMPLES);

    printf("ADC(mV, %d samples) = %d \n", LDR_SAMPLES, ldr_mv);

    if ((light && ldr_mv < (LDR_THRESHOLD - LDR_DELTA)) ||
        (!light && ldr_mv > (LDR_THRESHOLD + LDR_DELTA))) {
      light = !light;
    }

    gpio_set_level(LED_GPIO, !light);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
