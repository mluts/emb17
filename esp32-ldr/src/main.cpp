#include <Arduino.h>

#define ADC_IN 4
#define LOOP_DELAY_MS 100
#define ADC_MAX 4095
#define V_REF 3300

#define ADC_RESOLUTION 12
#define ADC_ATTENUATION ADC_6db

String attenuation_name(adc_attenuation_t atten) {
  switch (atten) {
  case ADC_ATTENDB_MAX:
    return "MAX";

  case ADC_11db:
    return "11db";

  case ADC_6db:
    return "6db";

  case ADC_2_5db:
    return "2.5db";

  case ADC_0db:
    return "0db";
  }

  return "";
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  delay(1000);

  pinMode(ADC_IN, INPUT);
  analogSetPinAttenuation(ADC_IN, ADC_ATTENUATION);

  Serial.printf("ADC_RESOLUTION = %d; ADC_ATTENUATIONa: %s", ADC_RESOLUTION,
                attenuation_name(ADC_ATTENUATION).c_str());

  Serial.println();
  Serial.println( "+-------+-----------+-----------+----------+---------+");
  Serial.println("|   RAW | calc (mV) | meas (mV) | err (mV) | err (%) |");
  Serial.println("+-------+-----------+-----------+----------+---------+");
}

uint16_t calcAdcMv(uint16_t adcVal) { return adcVal * V_REF / ADC_MAX; }

void printVoltages(uint16_t rawAdcVal, uint16_t rawAdcValmv,
                   uint16_t adcValMv) {
  uint16_t mvError = abs((int)rawAdcValmv - (int)adcValMv);
  float mvErrorPct = 0.0;

  if (adcValMv > 0) {
    mvErrorPct = mvError * 100.0f / adcValMv;
  }

  Serial.printf("| %5u | %9u | %9u | %8u | %7.2f |\n", rawAdcVal, rawAdcValmv,
                adcValMv, mvError, mvErrorPct);
}

void loop() {
  static uint16_t adcVal, adcValmv;
  uint16_t newAdcVal = analogRead(ADC_IN);

  if (newAdcVal != adcVal) { // At least skip same rows
    adcValmv = analogReadMilliVolts(ADC_IN);
    adcVal = newAdcVal;

    printVoltages(adcVal, calcAdcMv(adcVal), adcValmv);
  }

  delay(LOOP_DELAY_MS);
}
