#include <Arduino.h>

constexpr int32_t EXHAUST_FAN_WORK_TIME_SECONDS = 60 * 15;  // 15 mins
constexpr int32_t EXHAUST_FAN_SLEEP_TIME_SECONDS = 60 * 60; // 1 hour

const uint8_t EXHAUST_FAN_OUTPUT_PIN = 16;

const uint8_t TIMER_NO = 0;

// 80MHz / 80 = 1MHz
//  -> makes 1_000_000 ticks per second
const uint32_t TIMER_DIVIDER = 80;

// 1 000 000 microseconds
const uint32_t TIMER_TICKS_PER_SECOND = 1000000;

volatile bool exhaustFanEnabled = false;
hw_timer_t *timer = NULL;

void ARDUINO_ISR_ATTR startTurnOffTimer() {
  timerWrite(timer, 0);
  timerAlarmWrite(timer, TIMER_TICKS_PER_SECOND * EXHAUST_FAN_WORK_TIME_SECONDS,
                  false);
  timerAlarmEnable(timer);
}

void ARDUINO_ISR_ATTR startTurnOnTimer() {
  timerWrite(timer, 0);
  timerAlarmWrite(timer, TIMER_TICKS_PER_SECOND * EXHAUST_FAN_SLEEP_TIME_SECONDS,
                  false);
  timerAlarmEnable(timer);
}

void ARDUINO_ISR_ATTR onExhaustFanTimer() {
  if (exhaustFanEnabled) {
    Serial.println("Turning off the fan");
    exhaustFanEnabled = false;
    digitalWrite(EXHAUST_FAN_OUTPUT_PIN, LOW);
    startTurnOnTimer();
  } else {
    Serial.println("Turning on the fan");
    exhaustFanEnabled = true;
    digitalWrite(EXHAUST_FAN_OUTPUT_PIN, HIGH);
    startTurnOffTimer();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP32 2.5 Exercise - Timers, Watchdog");
  Serial.printf(
      "Automatically turn-on exhaust fan for %d mins every %f.02 hour\n",
      EXHAUST_FAN_WORK_TIME_SECONDS / 60,
      EXHAUST_FAN_SLEEP_TIME_SECONDS / (float)(60 * 60));

  // Initialize exhaust fan pin
  pinMode(EXHAUST_FAN_OUTPUT_PIN, OUTPUT);

  timer = timerBegin(TIMER_NO, TIMER_DIVIDER, true);
  timerAttachInterrupt(timer, onExhaustFanTimer, true);

  onExhaustFanTimer();
}

void loop() {}
