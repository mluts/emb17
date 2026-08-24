#include <Arduino.h>

enum class LedState {
  On,
  Off,
};

enum class LedMode {
  On,
  Blinking,
};

class Led {
private:
  LedState curState = LedState::Off;
  LedMode curMode = LedMode::Blinking;

public:
  static constexpr uint8_t LED_OUT = 15;
  static constexpr unsigned long BLINK_INTERVAL_MS = 1000;

  void init() {
    pinMode(LED_OUT, OUTPUT);
    set(LedState::Off);
  }

  void set(LedState state) {
    curState = state;

    if (curState == LedState::On) {
      digitalWrite(LED_OUT, HIGH);
    } else {
      digitalWrite(LED_OUT, LOW);
    }
  }

  LedState get() { return curState; }

  void setMode(LedMode mode) { curMode = mode; }

  LedMode getMode() { return curMode; }
};

constexpr uint8_t BTN_PIN = 16;

volatile bool buttonPressed = false;

void buttonPressedHandler() {
  if (digitalRead(BTN_PIN) == LOW) {
    buttonPressed = true;
  } else {
    buttonPressed = false;
  }
}

Led led;

void setup() {
  Serial.begin(115200);

  Serial.println();
  Serial.println("ESP32 LED C++ Exercise");
  Serial.println("Voltage(V) | LED");

  led.init();

  pinMode(BTN_PIN, INPUT_PULLUP);
  attachInterrupt(BTN_PIN, buttonPressedHandler, CHANGE);
}

void loop() {
  static unsigned long lastBlink = 0; // Set to zero just in case.
  static unsigned long iterationStartMicros, iterations;

  iterationStartMicros = micros();

  iterations++;

  if (buttonPressed) {
    // Serial.println("button pressed");
    if (led.getMode() == LedMode::Blinking) {
      led.setMode(LedMode::On);
    } else {
      led.setMode(LedMode::Blinking);
    }
  }

  if (led.getMode() == LedMode::Blinking &&
      ((millis() - lastBlink) >= Led::BLINK_INTERVAL_MS)) {
    lastBlink = millis();

    if (led.get() == LedState::On) {
      Serial.println("Led off");
      led.set(LedState::Off);
    } else {
      Serial.println("Led on");
      led.set(LedState::On);
    }
  } else if (led.getMode() == LedMode::On) {
    led.set(LedState::On);
  }

  if ((iterations % 10000) == 0) { // every 1000 iteration
    Serial.printf("Iteration time: %lu (micros)\n",
                  micros() - iterationStartMicros);
  }
}
