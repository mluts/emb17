#include <Arduino.h>

enum class LedState {
  On,
  Off,
};

class Led {
public:
  LedState curState = LedState::Off;

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
};

Led led;

void setup() {
  Serial.begin(115200);

  Serial.println();
  Serial.println("ESP32 LED C++ Exercise");
  Serial.println("Voltage(V) | LED");

  led.init();
}

void loop() {
  static unsigned long lastBlink = 0; // Set to zero just in case.
  static unsigned long iterations, iteration_ms;

  iteration_ms = millis() - iteration_ms;
  iterations++;

  if ((iterations % 1000) == 0) { // every 1000 iteration
    Serial.printf("Iteration time: %lu", iteration_ms);
  }

  if ((millis() - lastBlink) >= Led::BLINK_INTERVAL_MS) {
    if (led.curState == LedState::On) {
      Serial.println("Led off");
      led.set(LedState::Off);
    } else {
      Serial.println("Led on");
      led.set(LedState::On);
    }
  }
}
