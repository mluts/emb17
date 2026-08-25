#include <Arduino.h>

// Convenience abstraction for running code after some time has elapsed
class Timer {
private:
  unsigned long delayMs;
  unsigned long startMs;

public:
  Timer(unsigned long delayMs) {
    this->delayMs = delayMs;
    restart();
  }

  Timer() { Timer(0); }

  bool isReady() { return (millis() - startMs) >= delayMs; }

  void restart() { startMs = millis(); }
};

// Convenience abstraction to measure time duration between start() and end()
class DelayMeasure {
private:
  unsigned long measureStartMicros, measureEndMicros;

  DelayMeasure(unsigned long startMs) { this->measureStartMicros = startMs; }

  DelayMeasure() {
    this->measureStartMicros = 0;
    this->measureEndMicros = 0;
  }

public:
  static DelayMeasure start() { return DelayMeasure(micros()); }

  static DelayMeasure empty() { return DelayMeasure(0); }

  bool isComplete() { return measureStartMicros != 0 && measureEndMicros != 0; }
  bool isEmpty() { return measureStartMicros == 0 && measureEndMicros == 0; }

  void end() {
    if (!isEmpty()) {
      this->measureEndMicros = micros();
    }
  }

  unsigned long getMeasure() { return measureEndMicros - measureStartMicros; }
};

// Handles signal coming from GPIO to relay
class RelayInput {
private:
  static uint8_t PIN;

  static bool state;

public:
  static void init(uint8_t pin, unsigned long newSwitchDelayMs) {
    PIN = pin;
    state = false;

    pinMode(PIN, OUTPUT);
    digitalWrite(PIN, LOW);
  }

  static bool getState() { return state; }

  static void doSwitch(bool newState) {
    state = newState;

    if (state) {
      digitalWrite(PIN, HIGH); // On true we're activating relay
    } else {
      digitalWrite(PIN, LOW); // On false we're disabling relay
    }
  }
};

uint8_t RelayInput::PIN;
bool RelayInput::state;

// Handles signal coming from relay to GPIO
class RelayOutput {
private:
  static uint8_t PIN;
  volatile static bool relayOn; // Changed in ISP

public:
  static void init(uint8_t pin) {
    PIN = pin;
    relayOn = false;

    pinMode(PIN, INPUT_PULLUP);
  }

  static uint8_t getPin() { return PIN; }

  static void interruptHandler() {
    if (digitalRead(PIN) == HIGH) {
      relayOn = false; // HIGH -> relay is closed
    } else {
      relayOn = true; // LOW -> relay is open
    }
  }

  static bool getRelayOn() { return relayOn; }
};

uint8_t RelayOutput::PIN;
volatile bool RelayOutput::relayOn;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP32 Relay Exercise");

  RelayInput::init(15, 1000); // We will activate relay from GPIO 15 (via BC547)

  RelayOutput::init(16); // Relay Output GPIO and Debounce MS

  attachInterrupt(RelayOutput::getPin(), RelayOutput::interruptHandler, CHANGE);
}

void loop() {
  static Timer relayInputTimer = Timer(1000); // Create restartable timer
  static bool isRelayOn = RelayOutput::getRelayOn();

  static DelayMeasure measure = DelayMeasure::empty();

  static unsigned long relayTimeMax, relayTimeMin;

  // NOTE: Assuming that initially `isRelayOn != RelayOutput::getRelayOn()`
  //       And becomes `isRelayOn == RelayOutput::getRelayOn()` on
  //       next loop() iterations
  if (!measure.isComplete() && isRelayOn == RelayOutput::getRelayOn()) {
    Serial.print("\n      ... measurement complete, ");
    measure.end();

    Serial.printf(
        "\n        Relay switch time: %lu(micros)\n",
        measure.getMeasure());
  }

  if (relayInputTimer.isReady()) {
    Serial.println();
    // Serial.println("{TIMER} Toggle relay input");
    Serial.print("Timer -> Toggle Relay, ");
    bool newRelayState = !RelayInput::getState();
    Serial.printf("\n  ...set relay to %d, ", newRelayState ? 1 : 0);
    isRelayOn = newRelayState;
    RelayInput::doSwitch(newRelayState);

    measure = DelayMeasure::start();
    Serial.print("\n    ...start measure output delay, ");

    relayInputTimer.restart();
  }
}
