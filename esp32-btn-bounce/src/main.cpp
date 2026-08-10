#include <Arduino.h>

#define BUTTON_LEFT 15
#define BUTTON_RIGHT 0

#define DELAY 10
#define DEBOUNCE_PERIOD 10

int16_t counter_left = 0;
int16_t counter_right = 0;

void IRAM_ATTR reaction_left() {
  counter_left++;
  // Serial.println("\nLEFT Button Pressed! Count: " + String(counter_left));
}

void IRAM_ATTR reaction_right() {
  counter_right++;
  // Serial.println("\nRIGHT Button Pressed! Count: " + String(counter_right));
}

void setup() {
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  // pinMode(BUTTON_RIGHT, INPUT);
  Serial.begin(115200);
  attachInterrupt(digitalPinToInterrupt(BUTTON_LEFT), reaction_left, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_RIGHT), reaction_right, FALLING);
}

void loop() {
  static int16_t left_count = counter_left, left_btn_presses = 0;
  static unsigned long ms = 0;
  static unsigned long last_update_at = 0;

  bool left_btn_pressed = digitalRead(BUTTON_LEFT) == LOW;

  if (left_count != counter_left &&
      (ms - last_update_at) > DEBOUNCE_PERIOD &&
      left_btn_pressed) {
    left_btn_presses++;
    Serial.printf("Left btn count: %d (times);"
                  "Bounces: %d (times);"
                  "Period %lu (ms)\n",
                  left_btn_presses, counter_left - left_count, ms - last_update_at);
    last_update_at = ms;
    left_count = counter_left;
  }


  ms += DELAY;

  delay(DELAY);
}
