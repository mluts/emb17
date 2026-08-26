#include <Arduino.h>

constexpr uint8_t LED_PIN = 15;
constexpr uint8_t BTN_PIN = 16;
constexpr unsigned long BLINK_INTERVAL_MS = 15;

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
  static LedState curState;
  static LedMode curMode;

public:
  static void init() {
    curState = LedState::Off;
    curMode = LedMode::Blinking;

    pinMode(LED_PIN, OUTPUT);
    set(LedState::Off);
  }

  static void set(LedState state) {
    curState = state;

    if (curState == LedState::On) {
      digitalWrite(LED_PIN, HIGH);
    } else {
      digitalWrite(LED_PIN, LOW);
    }
  }

  static LedState get() { return curState; }

  static void setMode(LedMode mode) { curMode = mode; }

  static LedMode getMode() { return curMode; }
};

LedState Led::curState;
LedMode Led::curMode;

class Btn {
private:
  static uint8_t pin;

  static volatile bool buttonPressed;

public:
  static void init() {
    pin = BTN_PIN;
    buttonPressed = false;
    pinMode(pin, INPUT_PULLUP);
    attachInterrupt(pin, Btn::buttonPressedHandler, CHANGE);
  }

  static void buttonPressedHandler() {
    if (digitalRead(BTN_PIN) == LOW) {
      buttonPressed = true;
    } else {
      buttonPressed = false;
    }
  }

  static bool isPressed() { return buttonPressed; }
};

uint8_t Btn::pin;
volatile bool Btn::buttonPressed;

void setup() {
  Serial.begin(115200);

  Serial.println();
  Serial.println("ESP32 LED C++ Exercise");
  Serial.println("Voltage(V) | LED");
}

void loop() {
  static unsigned long lastBlink = 0; // Set to zero just in case.
  static unsigned long iterationStartMicros, iterations;

  iterationStartMicros = micros();

  iterations++;

  if (Btn::isPressed()) {
    if (Led::getMode() == LedMode::Blinking) {
      Led::setMode(LedMode::On);
    } else {
      Led::setMode(LedMode::Blinking);
    }
  }

  if (Led::getMode() == LedMode::Blinking &&
      ((millis() - lastBlink) >= BLINK_INTERVAL_MS)) {
    lastBlink = millis();

    if (Led::get() == LedState::On) {
      Serial.println("Led off");
      Led::set(LedState::Off);
    } else {
      Serial.println("Led on");
      Led::set(LedState::On);
    }
  } else if (Led::getMode() == LedMode::On) {
    Led::set(LedState::On);
  }

  if ((iterations % 10000) == 0) { // every 1000 iteration
    Serial.printf("Iteration time: %lu (micros)\n",
                  micros() - iterationStartMicros);
  }
}
