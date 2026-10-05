
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

#define BUZZER_GPIO 5
#define BUZZER_LEDC_CHANNEL LEDC_CHANNEL_0

// LEDC
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_RESOLUTION LEDC_TIMER_12_BIT
#define LEDC_FREQ_HZ 18000

#define LEDC_MAX_DUTY ((1 << LEDC_RESOLUTION) - 1)

#define SINE_TABLE_SIZE 256

// Визначення нот (частоти в Гц)
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523

#define BEAT_1_4_MS 600 // 100 bpm
#define BEAT_1_8_MS BEAT_1_4_MS / 2
#define BEAT_1_16_MS BEAT_1_8_MS / 2

#define PAUSE 0 // set_tone(0) freezes the phase, so the buzzer goes silent

struct Note {
  int freq_hz;
  int duration_ms;
};

// Baby Shark: C D E | C D E | C D E
const Note melody[] = {
    {NOTE_C4, BEAT_1_8_MS}, {NOTE_D4, BEAT_1_8_MS}, {NOTE_E4, BEAT_1_4_MS}, {PAUSE, BEAT_1_4_MS},
    {NOTE_C4, BEAT_1_8_MS}, {NOTE_D4, BEAT_1_8_MS}, {NOTE_E4, BEAT_1_4_MS}, {PAUSE, BEAT_1_4_MS},
    {NOTE_C4, BEAT_1_8_MS}, {NOTE_D4, BEAT_1_8_MS}, {NOTE_E4, BEAT_1_4_MS}, {PAUSE, BEAT_1_4_MS},
};
const int melody_count = sizeof(melody) / sizeof(melody[0]);

// Коректна синусоїда для 12-бітного PWM
const uint32_t sine_table[SINE_TABLE_SIZE] = {
    2048, 2098, 2148, 2198, 2248, 2298, 2348, 2398, 2447, 2496, 2545, 2594,
    2642, 2690, 2737, 2784, 2831, 2877, 2923, 2968, 3013, 3057, 3100, 3143,
    3185, 3226, 3267, 3307, 3346, 3385, 3423, 3459, 3495, 3530, 3565, 3598,
    3630, 3662, 3692, 3722, 3750, 3777, 3804, 3829, 3853, 3876, 3898, 3919,
    3939, 3958, 3975, 3992, 4007, 4021, 4034, 4045, 4056, 4065, 4073, 4080,
    4085, 4089, 4093, 4094, 4095, 4094, 4093, 4089, 4085, 4080, 4073, 4065,
    4056, 4045, 4034, 4021, 4007, 3992, 3975, 3958, 3939, 3919, 3898, 3876,
    3853, 3829, 3804, 3777, 3750, 3722, 3692, 3662, 3630, 3598, 3565, 3530,
    3495, 3459, 3423, 3385, 3346, 3307, 3267, 3226, 3185, 3143, 3100, 3057,
    3013, 2968, 2923, 2877, 2831, 2784, 2737, 2690, 2642, 2594, 2545, 2496,
    2447, 2398, 2348, 2298, 2248, 2198, 2148, 2098, 2048, 1997, 1947, 1897,
    1847, 1797, 1747, 1697, 1648, 1599, 1550, 1501, 1453, 1405, 1358, 1311,
    1264, 1218, 1172, 1127, 1082, 1038, 995,  952,  910,  869,  828,  788,
    749,  710,  672,  636,  600,  565,  530,  497,  465,  433,  403,  373,
    345,  318,  291,  266,  242,  219,  197,  176,  156,  137,  120,  103,
    88,   74,   61,   50,   39,   30,   22,   15,   10,   6,    2,    1,
    0,    1,    2,    6,    10,   15,   22,   30,   39,   50,   61,   74,
    88,   103,  120,  137,  156,  176,  197,  219,  242,  266,  291,  318,
    345,  373,  403,  433,  465,  497,  530,  565,  600,  636,  672,  710,
    749,  788,  828,  869,  910,  952,  995,  1038, 1082, 1127, 1172, 1218,
    1264, 1311, 1358, 1405, 1453, 1501, 1550, 1599, 1648, 1697, 1747, 1797,
    1847, 1897, 1947, 1997};

// // Кількість точок у таблиці синуса
// #define SINE_TABLE_SIZE 64
//
// // Коректна синусоїда для 12-бітного PWM
// const uint16_t sine_table[SINE_TABLE_SIZE] = {
//     2048, 2248, 2447, 2642, 2831, 3013, 3185, 3347,
//     3496, 3631, 3750, 3854, 3939, 4007, 4056, 4085,
//     4095, 4085, 4056, 4007, 3939, 3854, 3750, 3631,
//     3496, 3347, 3185, 3013, 2831, 2642, 2447, 2248,
//     2048, 1847, 1648, 1453, 1264, 1082, 910, 748,
//     599, 464, 345, 241, 156, 88, 39, 10,
//     0, 10, 39, 88, 156, 241, 345, 464,
//     599, 748, 910, 1082, 1264, 1453, 1648, 1847
// };

void configure_ledc(uint8_t gpio_num) {
  // Initializing LEDC timer
  ledc_timer_config_t ledc_timer = {.speed_mode = LEDC_MODE,
                                    .duty_resolution = LEDC_RESOLUTION,
                                    .timer_num = LEDC_TIMER,
                                    .freq_hz = LEDC_FREQ_HZ,
                                    .clk_cfg = LEDC_AUTO_CLK,
                                    .deconfigure = false};
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  ledc_channel_config_t ledc_ch0 = {
      .gpio_num = gpio_num,
      .speed_mode = LEDC_MODE,
      .channel = BUZZER_LEDC_CHANNEL,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = LEDC_TIMER,
      .duty = 0,
      .hpoint = 0,
      .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
      .flags = {.output_invert = 0},
      .deconfigure = false,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_ch0));
}

static gptimer_handle_t sample_timer = NULL;

// NOTE: no floats here — FPU use inside an ISR is forbidden on ESP32-S3 and
// crashes the board. phase is kept in thousandths of a table index instead.
#define PHASE_SCALE 1000

static volatile uint32_t phase = 0;
static volatile uint32_t phase_step = 0;

void set_tone(int freq_hz) {
  // table entries to advance per sample, times PHASE_SCALE
  phase_step = (uint32_t)SINE_TABLE_SIZE * freq_hz * PHASE_SCALE / LEDC_FREQ_HZ;
}

static bool IRAM_ATTR on_sample_alarm(gptimer_handle_t timer,
                                      const gptimer_alarm_event_data_t *edata,
                                      void *user_ctx) {
  phase += phase_step;
  if (phase >= SINE_TABLE_SIZE * PHASE_SCALE) {
    phase -= SINE_TABLE_SIZE * PHASE_SCALE;
  }
  ledc_set_duty(LEDC_MODE, BUZZER_LEDC_CHANNEL, sine_table[phase / PHASE_SCALE]);
  ledc_update_duty(LEDC_MODE, BUZZER_LEDC_CHANNEL);
  return false;
}

void configure_sample_timer() {
  gptimer_config_t timer_config = {
      .clk_src = GPTIMER_CLK_SRC_DEFAULT,
      .direction = GPTIMER_COUNT_UP,
      .resolution_hz = LEDC_FREQ_HZ,
      .intr_priority = 0,
      .flags = {.intr_shared = 0, .allow_pd = 0},
  };
  ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &sample_timer));

  gptimer_event_callbacks_t cbs = {.on_alarm = on_sample_alarm};
  ESP_ERROR_CHECK(gptimer_register_event_callbacks(sample_timer, &cbs, NULL));

  gptimer_alarm_config_t alarm_config = {
      .alarm_count = 1,
      .reload_count = 0,
      .flags = {.auto_reload_on_alarm = 1},
  };
  ESP_ERROR_CHECK(gptimer_set_alarm_action(sample_timer, &alarm_config));
  ESP_ERROR_CHECK(gptimer_enable(sample_timer));
  ESP_ERROR_CHECK(gptimer_start(sample_timer));
}

extern "C" void app_main() {
  configure_ledc(BUZZER_GPIO);
  configure_sample_timer();

  while (1) {
    for (int i = 0; i < melody_count; i++) {
      set_tone(melody[i].freq_hz * 4); // ~two octaves up, so piezo is louder there
      vTaskDelay(pdMS_TO_TICKS(melody[i].duration_ms));
    }
  }
}
