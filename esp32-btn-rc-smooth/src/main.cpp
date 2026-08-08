#include <Arduino.h>

#define GPIO_BTN1 15
#define GPIO_BTN2 0

#define GPIO_LED1 16
#define GPIO_LED2 17

#define BTN1 1
#define BTN2 2

#define POLL_RATE 50

#define LEDS_MAX_TIME 2000

typedef unsigned long TimeMs;

typedef enum {
  BTN1_RELEASED,
  BTN12_PRESSED,
  // BTN1_LONGPRESSED,
  BTN2_RELEASED,
  // BTN2_LONGPRESSED,
} Event_t;

typedef enum {
  LEDS_OFF,
  LEDS_ON,
  LED1_ON_ONLY,
  LED2_ON_ONLY,
  LEDS_CHANGE,
  LEDS_CHANGE_TOGETHER
} LedsMode_t;

struct {
  int speed = 1;
  LedsMode_t mode = LEDS_OFF;
  int pins[2] = {GPIO_LED1, GPIO_LED2};

  unsigned long lastChangeAt = 0;
  int lastChangeState = 0;
  int lead = 0;
} ledsConfig;

struct myBtn_t {
  bool pressed = false;
  TimeMs since = 0;
  bool eventHandled = false;
  bool skipNext = false;
  int pin;
};

myBtn_t btn1, btn2;

void myBtn_process(myBtn_t* b) {
  bool v = digitalRead(b->pin) == LOW ? true : false;

  if (b->pressed != v) {
    b->since = millis();
    b->pressed = v;
    b->eventHandled = false;
  }
}

int clamp(int a, int v, int b) {
  return v < a ? a : (v > b ? b : v);
}

void setLeds(bool led1, bool led2) {
  digitalWrite(GPIO_LED1, led1 ? HIGH : LOW);
  digitalWrite(GPIO_LED2, led2 ? HIGH : LOW);
}

void handleLedsMode(){
  unsigned long ms = millis();

  switch (ledsConfig.mode) {
    case LEDS_OFF:
      // Serial.println("LEDS OFF");
      setLeds(false, false);
      break;

    case LEDS_ON:
      // Serial.println("LEDS ON");
      setLeds(true, true);
      break;

    case LED1_ON_ONLY:
      // Serial.println("LEDS LED 1 ONLY");
      setLeds(true, false);
      break;

    case LED2_ON_ONLY:
      // Serial.println("LEDS LED 2 ONLY");
      setLeds(false, true);
      break;

    case LEDS_CHANGE_TOGETHER:
      Serial.println("LEDS CHANGE TOGETHER");
      if ((ms - ledsConfig.lastChangeAt) > LEDS_MAX_TIME / (5 * clamp(1, ledsConfig.speed, 10))) {

        if (ledsConfig.lastChangeState % 2 == 0) {
          setLeds(true, true);
        } else {
          setLeds(false, false);
        }

        ledsConfig.lastChangeState++;
        ledsConfig.lastChangeAt = ms;
      }
      break;

    case LEDS_CHANGE:
      // Serial.println("LEDS CHANGE");
      if ((ms - ledsConfig.lastChangeAt) > LEDS_MAX_TIME / (2 * clamp(1, ledsConfig.speed, 10))) {

        if (ledsConfig.lastChangeState % 2 == 0) {
          setLeds(true, false);
        } else {
          setLeds(false, true);
        }

        ledsConfig.lastChangeState++;
        ledsConfig.lastChangeAt = ms;
      }
      break;
  }
}

void led1Lead() {
  if (ledsConfig.lead != 1) {
    ledsConfig.lead = 1;
    ledsConfig.mode = LEDS_OFF;
    return;
  }

  switch(ledsConfig.mode) {
    case LEDS_CHANGE_TOGETHER:
    case LED2_ON_ONLY:
    case LEDS_OFF:
      ledsConfig.mode = LED1_ON_ONLY;
      break;

    case LED1_ON_ONLY:
      ledsConfig.mode = LEDS_ON;
      break;

    case LEDS_CHANGE:
      ledsConfig.speed++;
      break;

    case LEDS_ON:
      ledsConfig.speed = 0;
      ledsConfig.mode = LEDS_CHANGE;
      break;
  }
}

void led2Lead() {
  if (ledsConfig.lead != 2) {
    ledsConfig.lead = 2;
    ledsConfig.mode = LEDS_OFF;
    return;
  }

  switch(ledsConfig.mode) {
    case LEDS_CHANGE_TOGETHER:
    case LED1_ON_ONLY:
    case LEDS_OFF:
      ledsConfig.mode = LED2_ON_ONLY;
      break;

    case LED2_ON_ONLY:
      ledsConfig.mode = LEDS_ON;
      break;

    case LEDS_CHANGE:
      ledsConfig.speed++;
      break;

    case LEDS_ON:
      ledsConfig.speed = 0;
      ledsConfig.mode = LEDS_CHANGE;
      break;
  }
}

void ledsChangeTogether() {
  if (ledsConfig.mode == LEDS_CHANGE_TOGETHER) {
    ledsConfig.speed++;
  } else {
    ledsConfig.speed = 0;
    ledsConfig.mode = LEDS_CHANGE_TOGETHER;
  }
}

void handleEvent(Event_t e) {
  switch (e) {
    case BTN1_RELEASED:
      Serial.println("btn1 released");
      led1Lead();
      break;

    case BTN2_RELEASED:
      Serial.println("btn2 released");
      led2Lead();
      break;

    case BTN12_PRESSED:
      Serial.println("btn12 pressed");
      ledsChangeTogether();
      break;
  }
}

void processBtns() {
  myBtn_process(&btn1);
  myBtn_process(&btn2);

  if (btn1.pressed && btn2.pressed && !btn1.eventHandled && !btn2.eventHandled) {
    btn1.eventHandled = true;
    btn2.eventHandled = true;
    btn1.skipNext = true;
    btn2.skipNext = true;
    handleEvent(BTN12_PRESSED);
  } else if (!btn1.pressed && !btn1.eventHandled) {
    if (!btn1.skipNext)  {
      handleEvent(BTN1_RELEASED);
    }
    btn1.skipNext = false;
    btn1.eventHandled = true;
  } else if (!btn2.pressed && !btn2.eventHandled) {
    if (!btn2.skipNext){
      handleEvent(BTN2_RELEASED);
    }
    btn2.skipNext = false;
    btn2.eventHandled = true;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(GPIO_BTN1, INPUT_PULLUP);
  pinMode(GPIO_LED1, OUTPUT);
  pinMode(GPIO_LED2, OUTPUT);

  btn1.pin = GPIO_BTN1;
  btn2.pin = GPIO_BTN2;
}

void loop() {
  processBtns();
  handleLedsMode();

  delay(POLL_RATE);
}
