#include <Arduino.h>

enum class LedMode { On, Off, Blinking };

enum class TrafficMode { Green, GreenBlinking, Yellow, Red, YellowAndRed, Off };

class TrafficModeStep {
private:
  TrafficMode mode;
  uint32_t totalSteps;
  uint32_t countSteps;

public:
  TrafficModeStep(TrafficMode mode, uint32_t totalSteps) {
    this->mode = mode;
    this->totalSteps = totalSteps;
    this->countSteps = 0;
  }

  // increment step
  // return true if reached totalSteps
  bool incrementSteps() {
    this->countSteps++;

    return this->countSteps >= this->totalSteps;
  }
};

class Led {

private:
  uint8_t ledPin;
  LedMode mode = LedMode::Off;
  unsigned long modeSetAt = 0;

  unsigned long blinkingPeriod = 1000;
  unsigned long blinkedAt = 0;
  bool blinkingState = false;

  void pinWrite() {
    switch (this->mode) {
    case LedMode::On:
      digitalWrite(this->ledPin, HIGH);
      break;

    case LedMode::Off:
      digitalWrite(this->ledPin, LOW);
      break;

    case LedMode::Blinking:
      if (this->blinkingState) {
        digitalWrite(this->ledPin, HIGH);
      } else {
        digitalWrite(this->ledPin, LOW);
      }
      break;
    }
  }

public:
  Led(uint8_t pin) { this->ledPin = pin; }

  void setup() { pinMode(this->ledPin, OUTPUT); }

  void setBlinkingPeriod(unsigned long period) {
    this->blinkingPeriod = period;
  }

  void setMode(LedMode newMode) {
    this->modeSetAt = millis();
    this->mode = newMode;
  }

  void update() {}
};

Led greenLed = Led(16);
Led yellowLed = Led(17);
Led redLed = Led(18);

TrafficModeStep steps[] = {
  TrafficModeStep(TrafficMode::Green, 10),
  TrafficModeStep(TrafficMode::GreenBlinking, 6),
  TrafficModeStep(TrafficMode::Yellow, 6),
};

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP32 2.5 Exercise - Timers, Watchdog");

  greenLed.setup();
  redLed.setup();
  yellowLed.setup();
}

void loop() {}
