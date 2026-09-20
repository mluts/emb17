#include "stm32f4xx_hal.h"
#include "main.h"

extern "C" const uint16_t VREFINT_CAL = *VREFINT_CAL_ADDR;

extern "C" ADC_HandleTypeDef hadc1;

extern "C" void main_cpp(void) {
  while (1) {
    if (HAL_ADC_Start(&hadc1) == HAL_OK) {
      if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
        int raw_ref = HAL_ADC_GetValue
        int raw = HAL_ADC_GetValue(&hadc1);
        int vdda = (VREFINT_CAL_VREF * VREFINT_CAL) / raw;
      }
    }
    HAL_Delay(100);
  }
}
