#include <Arduino.h>

class Led {

private:
  uint8_t ledPin;

public:
  Led(uint8_t pin) { this->ledPin = pin; }

  void setup() { pinMode(this->ledPin, OUTPUT); }
};

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP32 2.5 Exercise - Timers, Watchdog");
}

void loop() {}
