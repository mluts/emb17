#include "encoder/encoder.h"
#include "main.h"
#include "pwm/pwm.h"
#include "servo/servo.h"
#include "sound/sound.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>

#define ENCODER_A_PORT GPIOA
#define ENCODER_A_PIN GPIO_PIN_0
#define ENCODER_B_PORT GPIOA
#define ENCODER_B_PIN GPIO_PIN_1

#define ENCODER_BUTTON_PORT GPIOA
#define ENCODER_BUTTON_PIN GPIO_PIN_2

#define ENCODER_DEBOUNCE_NS 2000000

#define ENCODER_PULSES_PER_STEP 4

#define SERVO_GPIO_PORT PWM_PORT_B
#define SERVO_GPIO_PIN 4

#define SOUND_FREQUENCY_HZ 2200
#define SOUND_GPIO_PORT PWM_PORT_B
#define SOUND_GPIO_PIN 6
#define SOUND_GPIO_TIMER PWM_TIM4

#define SERVO_MIN_ROTATION_ANGLE 1
#define BUTTON_LONG_PRESS_MS 1000
#define BUTTON_DEBOUNCE_MS 20
#define BEEP_MS 300

typedef enum {
  RotationMode1 = 0,
  RotationMode2,
  RotationMode3,
  RotationState_Count,
} RotationState_t;

typedef enum {
  ButtonEvent_None = 0,
  ButtonEvent_ShortPress,
  ButtonEvent_LongPress,
} ButtonEvent_t;

// Returns current encoder position relative to initial 0
int32_t read_position(EncoderCtx_t *ctx) {
  static int32_t position = 0;
  Encoder_Read(ctx, &position);
  return position;
}

// return number in [min; v; max]
int32_t clamp(int32_t min, int32_t v, int32_t max) {
  if (v < min) {
    return min;
  }
  if (v > max) {
    return max;
  }
  return v;
}

bool get_button_pressed(EncoderCtx_t *encoder) {
  static bool raw_prev = false;
  static uint32_t raw_change_tick = 0;
  static bool stable = false;

  bool raw = (HAL_GPIO_ReadPin(encoder->button_port, encoder->button_pin) ==
              GPIO_PIN_RESET);

  if (raw != raw_prev) {
    raw_prev = raw;
    raw_change_tick = HAL_GetTick();
  }

  if ((HAL_GetTick() - raw_change_tick) >= BUTTON_DEBOUNCE_MS) {
    stable = raw;
  }

  return stable;
}

ButtonEvent_t handle_button(EncoderCtx_t *encoder) {
  static bool button_prev = false;
  static uint32_t press_start_tick = 0;
  static bool long_press_fired = false;

  bool button_pressed = get_button_pressed(encoder);

  ButtonEvent_t event = ButtonEvent_None;

  if (button_prev != button_pressed) {
    printf("button_pressed=%d\n", button_pressed);
  }

  if (button_pressed && !button_prev) {
    press_start_tick = HAL_GetTick();
    long_press_fired = false;
  } else if (!button_pressed && button_prev) {
    if (!long_press_fired) {
      event = ButtonEvent_ShortPress;
    }
    press_start_tick = 0;
  } else if (button_pressed && button_prev && !long_press_fired &&
             (HAL_GetTick() - press_start_tick) >= BUTTON_LONG_PRESS_MS) {
    long_press_fired = true;
    event = ButtonEvent_LongPress;
  }

  button_prev = button_pressed;
  return event;
}

int32_t get_rotation_angle(ButtonEvent_t event) {
  static RotationState_t rotation = RotationMode1;

  if (event == ButtonEvent_ShortPress) {
    rotation = (RotationState_t)((rotation + 1) % RotationState_Count);
    printf("RotationState=%d\n", rotation);
  }


  switch (rotation) {
    case RotationMode2:
      return 10;
    case RotationMode3:
      return 30;
    case RotationMode1:
    default:
      return SERVO_MIN_ROTATION_ANGLE;
  }
}

void configure_encoder(EncoderCtx_t *ctx, bool *error) {
  if (!Encoder_Init(ctx, ENCODER_A_PORT, ENCODER_A_PIN, ENCODER_B_PORT,
                    ENCODER_B_PIN, ENCODER_BUTTON_PORT, ENCODER_BUTTON_PIN,
                    ENCODER_DEBOUNCE_NS)) {
    *error = true;
    printf("Encoder init failed\n");
  }
}

void configure_servo(Servo_t *servo, PwmDriver_t *pwm, bool *error) {
  if (*error) {
    return;
  }

  if (!Pwm_InitByPin(pwm, SERVO_GPIO_PORT, SERVO_GPIO_PIN, SERVO_FREQUENCY_HZ,
                     0)) {
    *error = true;
    printf("Servo PWM init failed\n");
    return;
  }

  if (!Servo_Init(servo, pwm)) {
    *error = true;
    printf("Servo init failed\n");
  }
}

void configure_sound(PwmDriver_t *pwm, bool *error) {
  if (*error) {
    return;
  }

  uint32_t pwm_frequency_hz = Sound_GetPwmFrequency(SOUND_FREQUENCY_HZ);
  if (!Pwm_InitByPinAndTimer(pwm, SOUND_GPIO_PORT, SOUND_GPIO_PIN,
                             SOUND_GPIO_TIMER, pwm_frequency_hz, 50)) {
    *error = true;
    printf("Sound PWM init failed\n");
    return;
  }

  if (!Sound_Init(pwm, SOUND_FREQUENCY_HZ)) {
    *error = true;
    printf("Sound init failed\n");
  }
}

extern "C" void main_cpp(void) {
  // Wait for serial interface setup
  HAL_Delay(1000);
  bool error = false;

  printf("Starting...\n");

  EncoderCtx_t encoder = {0};

  PwmDriver_t servo_pwm = {};
  Servo_t servo = {0};

  PwmDriver_t sound_pwm = {};

  configure_encoder(&encoder, &error);

  configure_servo(&servo, &servo_pwm, &error);

  configure_sound(&sound_pwm, &error);

  Sound_Stop();
  bool sound_playing = false;
  uint32_t beep_start_tick = 0;
  bool at_limit_prev = false;

  // Start from center position
  int32_t angle = SERVO_MAX_ANGLE / 2;
  int32_t prev_steps = 0;

  if (!error) {
    printf("offset: %d\n", (int)(angle - SERVO_MAX_ANGLE / 2));
  }

  while (1) {
    if (error) {
      printf("Error was detected, please look for error messages\n");
      HAL_Delay(10000);
      continue;
    }

    ButtonEvent_t button_event = handle_button(&encoder);

    int32_t pulses = read_position(&encoder);
    int32_t steps = pulses / ENCODER_PULSES_PER_STEP;

    if (button_event == ButtonEvent_LongPress) {
      prev_steps = steps;
      angle = SERVO_MAX_ANGLE / 2;
      Servo_SetAngle(&servo, (uint16_t)angle);
      printf("recalibrated: encoder origin set to center\n");
    }

    int32_t step_angle = get_rotation_angle(button_event);
    int32_t delta_steps = steps - prev_steps;

    if (delta_steps != 0) {
      prev_steps = steps;
      int32_t new_angle =
          clamp(SERVO_MIN_ANGLE, angle - delta_steps * step_angle,
                SERVO_MAX_ANGLE);
      if (new_angle != angle) {
        angle = new_angle;
        Servo_SetAngle(&servo, (uint16_t)angle);
        printf("position: %d; offset: %d\n", (int)steps,
               (int)(angle - SERVO_MAX_ANGLE / 2));
      }
    }

    bool at_limit = (angle == SERVO_MIN_ANGLE || angle == SERVO_MAX_ANGLE);

    if (at_limit && !at_limit_prev && !sound_playing) {
      Sound_Init(&sound_pwm, SOUND_FREQUENCY_HZ);
      sound_playing = true;
      beep_start_tick = HAL_GetTick();
    }

    if (sound_playing && (HAL_GetTick() - beep_start_tick) >= BEEP_MS) {
      Sound_Stop();
      sound_playing = false;
    }

    at_limit_prev = at_limit;

    HAL_Delay(10);
  }
}
