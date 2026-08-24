#include <Arduino.h>

#define LED_OUT 15

void setup() {
    Serial.begin(115200);
    pinMode(LED_OUT, OUTPUT);

    Serial.println();
    Serial.println("ESP32 LED C++ Exercise");
    Serial.println("Voltage(V) | LED");
}

void loop() {
    delay(1000);
}
