#include <Arduino.h>

constexpr int32_t EXHAUST_FAN_WORK_TIME_SECONDS = 60 * 15;  // 15 mins
constexpr int32_t EXHAUST_FAN_SLEEP_TIME_SECONDS = 60 * 60; // 1 hour

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP32 2.5 Exercise - Timers, Watchdog");
  Serial.printf(
      "Automatically turn-on exhaust fan for %d mins every %f.02 hour",
      EXHAUST_FAN_WORK_TIME_SECONDS / 60,
      EXHAUST_FAN_SLEEP_TIME_SECONDS / (float)(60 * 60));
}

void loop() {
}
