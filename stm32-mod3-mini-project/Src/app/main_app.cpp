#include "encoder/encoder.h"
#include "main.h"
#include "pwm/pwm.h"
#include "servo/servo.h"
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
#define SERVO_STEP_ANGLE 5

int32_t read_position(EncoderCtx_t *ctx) {
  static int32_t position = 0;
  Encoder_Read(ctx, &position);
  return position;
}

int32_t clamp_angle(int32_t angle) {
  if (angle < SERVO_MIN_ANGLE) {
    return SERVO_MIN_ANGLE;
  }
  if (angle > SERVO_MAX_ANGLE) {
    return SERVO_MAX_ANGLE;
  }
  return angle;
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
  if (!*error && !Servo_Init(servo, pwm)) {
    *error = true;
    printf("Servo init failed\n");
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

  configure_encoder(&encoder, &error);

  configure_servo(&servo, &servo_pwm, &error);

  // Start from leftmost position
  int32_t angle = SERVO_MIN_ANGLE;

  if (!error) {
    printf("offset: %d\n", (int)(angle - SERVO_MIN_ANGLE));
  }

  while (1) {
    if (error) {
      printf("Error was detected, please look for error messages\n");
      HAL_Delay(10000);
      continue;
    }

    int32_t steps = read_position(&encoder) / ENCODER_PULSES_PER_STEP;
    int32_t new_angle = clamp_angle(SERVO_MIN_ANGLE + steps * SERVO_STEP_ANGLE);

    if (new_angle != angle) {
      angle = new_angle;
      Servo_SetAngle(&servo, (uint16_t)angle);
      printf("position: %d; offset: %d\n", steps,
             (int)(angle - SERVO_MIN_ANGLE));
    }

    HAL_Delay(10);
  }
}
