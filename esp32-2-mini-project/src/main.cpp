#include <Arduino.h>
#include <tuple>

const uint8_t GREEN_PIN_OUT = 15;
const uint8_t YELLOW_PIN_OUT = 16;
const uint8_t RED_PIN_OUT = 17;

const uint32_t MAX_PROGRAM_STEPS = 100;
const uint8_t TIMER_NO = 0;
// 80MHz / 80 = 1MHz
//  -> makes 1_000_000 ticks per second
const uint32_t TIMER_DIVIDER = 80;

const unsigned long TIMER_TICK_MS = 500;

hw_timer_t *timer = NULL;

enum class TrafficMode { Green, Yellow, Red, YellowAndRed, Off };

class TrafficProgram {
private:
  std::tuple<TrafficMode, unsigned long> steps[MAX_PROGRAM_STEPS];
  uint32_t stepsCount = 0;

  // This is used by ISR
  volatile uint32_t currentTrafficModeStep = 0;
  volatile unsigned long currentTrafficModeStepMs = 0;

  uint8_t greenPin = NULL;
  uint8_t redPin = NULL;
  uint8_t yellowPin = NULL;
  bool pinsInitialized = false;

public:
  void addProgramStep(std::tuple<TrafficMode, unsigned long> step) {
    if (stepsCount >= MAX_PROGRAM_STEPS) {
      return;
    }

    this->steps[this->stepsCount] = step;
    this->stepsCount++;
  }

  void setupPins(uint8_t greenPin, uint8_t yellowPin, uint8_t redPin) {
    this->greenPin = greenPin;
    this->yellowPin = yellowPin;
    this->redPin = redPin;

    pinMode(this->greenPin, OUTPUT);
    pinMode(this->yellowPin, OUTPUT);
    pinMode(this->redPin, OUTPUT);

    this->pinsInitialized = true;
  }

  void writePins() {
    if (this->stepsCount == 0 || !this->pinsInitialized) {
      return;
    }

    switch (std::get<0>(this->steps[this->currentTrafficModeStep])) {
    case TrafficMode::Green:
      digitalWrite(this->greenPin, HIGH);
      digitalWrite(this->yellowPin, LOW);
      digitalWrite(this->redPin, LOW);
      break;

    case TrafficMode::Off:
      digitalWrite(this->greenPin, LOW);
      digitalWrite(this->yellowPin, LOW);
      digitalWrite(this->redPin, LOW);
      break;

    case TrafficMode::YellowAndRed:
      digitalWrite(this->greenPin, LOW);
      digitalWrite(this->yellowPin, HIGH);
      digitalWrite(this->redPin, HIGH);
      break;

    case TrafficMode::Red:
      digitalWrite(this->greenPin, LOW);
      digitalWrite(this->yellowPin, LOW);
      digitalWrite(this->redPin, HIGH);
      break;

    case TrafficMode::Yellow:
      digitalWrite(this->greenPin, LOW);
      digitalWrite(this->yellowPin, HIGH);
      digitalWrite(this->redPin, LOW);
      break;
    }
  }

  void ARDUINO_ISR_ATTR tick(unsigned long msPassed) {
    if (this->stepsCount == 0) {
      return;
    }

    unsigned long curStepDuration =
        std::get<1>(this->steps[this->currentTrafficModeStep]);

    this->currentTrafficModeStepMs += msPassed;

    // when current step is overdue
    if (this->currentTrafficModeStepMs >= curStepDuration) {
      // reset next step time
      this->currentTrafficModeStepMs = 0;
      // `+ 1` switches to next step
      // `% this->stepsCount` switches to first step
      this->currentTrafficModeStep =
          (this->currentTrafficModeStep + 1) % this->stepsCount;
    }
  }
};

TrafficProgram program = TrafficProgram();

void ARDUINO_ISR_ATTR onTimer() { program.tick(TIMER_TICK_MS); }

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP32 Mini Project - Traffic Lights");

  // 5 seconds green
  program.addProgramStep({TrafficMode::Green, 5000});

  // 3 seconds green blinking
  program.addProgramStep({TrafficMode::Off, 500});
  program.addProgramStep({TrafficMode::Green, 500});
  program.addProgramStep({TrafficMode::Off, 500});
  program.addProgramStep({TrafficMode::Green, 500});
  program.addProgramStep({TrafficMode::Off, 500});
  program.addProgramStep({TrafficMode::Green, 500});

  // 2 seconds yellow
  program.addProgramStep({TrafficMode::Yellow, 2000});

  // 5 seconds red
  program.addProgramStep({TrafficMode::Red, 5000});

  // 2 seconds yellow and red (get ready)
  program.addProgramStep({TrafficMode::YellowAndRed, 2000});

  program.setupPins(GREEN_PIN_OUT, YELLOW_PIN_OUT, RED_PIN_OUT);

  timer = timerBegin(TIMER_NO, TIMER_DIVIDER, true);
  timerAttachInterrupt(timer, onTimer, true);
  timerAlarmWrite(timer, TIMER_TICK_MS * 1000, true);
  timerAlarmEnable(timer);
}

void loop() {
  program.writePins();
  delay(10);
}
