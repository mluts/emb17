#include <Arduino.h>

typedef enum {
  NONE,
  LED1_ON,
  LED2_ON,
  LED3_ON,
} ledsState_t;

#define LED1 15
#define LED2 16
#define LED3 17
#define TOTAL_LEDS 3
#define STEP_DELAY_INIT 500
#define STEP_DELAY_MIN 10

int leds[] = {
  LOW,  LOW,  LOW,
  LED1, LED2, LED3
};

int delay_state = STEP_DELAY_INIT;

ledsState_t state = NONE;

ledsState_t transitions[] = {LED1_ON, LED2_ON, LED3_ON,
  // LED2_ON, LED3_ON
};
int transitions_step = 0;

unsigned long last_millis = 0;
int dt = 0;

void setup() {
  Serial.begin(115200);
  delay(500);

  for (int i=TOTAL_LEDS; i<TOTAL_LEDS*2;i++) {
    pinMode(leds[i], OUTPUT);
  }
}

void calcDelta() {
  unsigned long ms = millis();
  dt = ms - last_millis;
  last_millis = ms;
  Serial.printf("delta = %d \n", dt);
}

void handleState() {
  switch (state) {
    case NONE:
      Serial.println("STATE = NONE");
      for (int i=0; i<TOTAL_LEDS; i++) { leds[i] = LOW; }
      break;

    case LED1_ON:
      Serial.println("STATE = LED1_ON");
      for (int i=0; i<TOTAL_LEDS; i++) { leds[i] = i == 0 ? HIGH : LOW; }
      break;

    case LED2_ON:
      Serial.println("STATE = LED2_ON");
      for (int i=0; i<TOTAL_LEDS; i++) { leds[i] = i == 1 ? HIGH : LOW; }
      break;

    case LED3_ON:
      Serial.println("STATE = LED3_ON");
      for (int i=0; i<TOTAL_LEDS; i++) { leds[i] = i == 2 ? HIGH : LOW; }
      break;
  }

  for (int i=0;i<TOTAL_LEDS;i++) { digitalWrite(leds[i+TOTAL_LEDS], leds[i]); }
}

void handleTransitions() {
  transitions_step %= sizeof(transitions) / sizeof(transitions[0]);
  Serial.printf("transitions_step = %d \n", transitions_step);
  state = transitions[transitions_step];
  transitions_step += 1;
}

void loop() {
  calcDelta();
  handleTransitions();
  handleState();

  delay_state = STEP_DELAY_MIN + STEP_DELAY_INIT + round(STEP_DELAY_INIT * cos(millis() / (PI * 750 / 2)));
  Serial.printf("delay = %d", delay_state);

  delay(delay_state);
}
