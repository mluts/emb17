#include <Arduino.h>

typedef enum {
  NONE,
  LED1,
  LED2,
  LED3
} ledsState_t;

#define LED1 15
#define LED2 16
#define LED3 17

ledsState_t ledState = NONE;

void setup() {
  Serial.begin(115200);
  delay(500);
}

void loop() {
  switch (ledState) {
    case NONE:
      break;

    case LED1:
      break;

    case LED2:
      break;

    case LED3:
      break;
  }

  delay(1000);
}
