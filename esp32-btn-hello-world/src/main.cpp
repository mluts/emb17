#include <Arduino.h>

#define GPIO_LED1 16
#define GPIO_LED2 17

#define GPIO_BTN1 15
#define GPIO_BTN1_NAME "GPIO 15"

#define GPIO_BTN2 0
#define GPIO_BTN2_NAME "GPIO 0 (BOOT)"

#define DEBOUNCE_TIMEOUT 10
#define POLL_TIME 10
#define STABLE_READ 50

#define LED1_BIT 1
#define LED2_BIT 2

#define LEDS_MAX_TIME 2000

#define COUNT_OF(arr) (sizeof(arr) / sizeof((arr)[0]))

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

struct btnTracker_t {
  unsigned long firstRead = 0;
  bool v = false;
  bool stable = true;
  bool handled = true;
  int pin;
};

btnTracker_t btn1, btn2;

btnTracker_t* btnTrackers[2];


bool debounced(volatile unsigned long* lastTime) {
  unsigned long ms = millis();

  if ((ms - *lastTime) > DEBOUNCE_TIMEOUT) {
    *lastTime = ms;
    return true;
  }

  return false;
}

volatile bool btn1Changed = false;
volatile bool btn2Changed = false;
volatile unsigned long btn1ChangedDb = 0;
volatile unsigned long btn2ChangedDb = 0;

void btn1Change(){ if (debounced(&btn1ChangedDb)) { btn1Changed = true; } }
void btn2Change(){ if (debounced(&btn2ChangedDb)) { btn2Changed = true; } }

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(GPIO_BTN1, INPUT_PULLUP);
  pinMode(GPIO_LED1, OUTPUT);
  pinMode(GPIO_LED2, OUTPUT);

  btn1.pin = GPIO_BTN1;
  btn2.pin = GPIO_BTN2;

  btnTrackers[0] = &btn1;
  btnTrackers[1] = &btn2;

  attachInterrupt(digitalPinToInterrupt(GPIO_BTN1), btn1Change, CHANGE);
  attachInterrupt(digitalPinToInterrupt(GPIO_BTN2), btn2Change, CHANGE);
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
      // Serial.println("btn1 released");
      led1Lead();
      break;

    case BTN2_RELEASED:
      // Serial.println("btn2 released");
      led2Lead();
      break;

    case BTN12_PRESSED:
      ledsChangeTogether();
      break;
  }
}

void trackBtn(btnTracker_t* t) {
  bool v = digitalRead(t->pin) == LOW ? true : false;
  unsigned long ms = millis();

  if ((t->firstRead > 0)
      && (t->v == v)
      && ((ms - t->firstRead) > STABLE_READ)) {
      // Serial.printf("BTN PIN: %d v=%d stable \n", t->pin, v);
    t->stable = true;
  } else if ((t->firstRead == 0) || t->v != v) {
    // if (t->firstRead == 0) {
    //   Serial.printf("BTN PIN: %d v=%d firstRead: %lu \n", t->pin, v, ms);
    // } else {
    //   Serial.printf("BTN PIN: %d unstable \n", t->pin);
    // }
    t->firstRead = ms;
    t->v = v;
    t->stable = false;
  }
}

void resetBtn(btnTracker_t* t) {
  t->handled = false;
  t->stable = false;
  t->firstRead = 0;
}

void handleInputs() {
  if (btn1Changed) {
    btn1Changed = false;
    resetBtn(btnTrackers[0]);
  }

  if (btn2Changed) {
    btn2Changed = false;
    resetBtn(btnTrackers[1]);
  }

  for (int i = 0; i < COUNT_OF(btnTrackers); i++) {
    if (!btnTrackers[i]->stable) {
      trackBtn(btnTrackers[i]);
    }
  }

  if (!btnTrackers[0]->handled && btnTrackers[0]->stable && btnTrackers[0]->v == true
      && !btnTrackers[1]->handled && btnTrackers[1]->stable && btnTrackers[1]->v == true) {
    handleEvent(BTN12_PRESSED);
    btnTrackers[0]->handled = true;
    btnTrackers[1]->handled = true;
  }

  if (!btnTrackers[0]->handled && btnTrackers[0]->stable && btnTrackers[0]->v == false) {
    handleEvent(BTN1_RELEASED);
    btnTrackers[0]->handled = true;
  }

  if (!btnTrackers[1]->handled && btnTrackers[1]->stable && btnTrackers[1]->v == false) {
    handleEvent(BTN2_RELEASED);
    btnTrackers[1]->handled = true;
  }
}

void loop() {
  handleInputs();
  handleLedsMode();

  delay(POLL_TIME);
}
