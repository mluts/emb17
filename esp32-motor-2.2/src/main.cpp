#include <Arduino.h>

constexpr uint32_t ADC_MAX_V = 3100;
constexpr unsigned long DUTY_CYCLE_PERIOD = 20;

class Motor {
private:
  static uint8_t motorPin;
  static bool on;

public:
  static void init(uint8_t pin) {
    motorPin = pin;
    pinMode(motorPin, OUTPUT);
    on = false;
    digitalWrite(motorPin, LOW);
  }

  static bool isOn() { return on; }

  static void set(bool newStatus) {
    if (newStatus) {
      digitalWrite(motorPin, HIGH);
    } else {
      digitalWrite(motorPin, LOW);
    }

    on = newStatus;
  }
};

uint8_t Motor::motorPin;
bool Motor::on;

class MotorSetting {
private:
  static uint8_t adcPin;

public:
  static void init(uint8_t pin) {
    adcPin = pin;
    pinMode(adcPin, INPUT);
  }

  static float dutyCycleRatio() {
    return (float)analogReadMilliVolts(adcPin) / ADC_MAX_V;
  }

  static unsigned long timeOn() {
    return round(dutyCycleRatio() * DUTY_CYCLE_PERIOD);
  }

  static unsigned long timeOff() {
    return round((1 - dutyCycleRatio()) * DUTY_CYCLE_PERIOD);
  }
};

uint8_t MotorSetting::adcPin;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP32 Motor PWM Exercise");

  Motor::init(15);
  MotorSetting::init(4);
}

void loop() {
  Motor::set(!Motor::isOn());

  if (Motor::isOn()) {
    delay(MotorSetting::timeOn());
  } else {
    delay(MotorSetting::timeOff());
  }
}
