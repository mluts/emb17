## Список GPIO

- PA0 - ADC - Потенціометр
- PA8 - TIM1_CH1 / PWM - DRV8833 IN1 через 817B
- PA9 - TIM1_CH2 / PWM - DRV8833 IN2 через 817B
- PB6 - TIM4_CH1 - KY040 - CLK
- PB7 - TIM4_CH2 - KY040 - DT

## PID

- Ku = ~150
- PID (без перерегулювання):
  - Kp = 30
  - Tu = 0.15
  - Ti = 0.075
  - Td = 0.05
  - Ki = Kp / Ti = 400
  - Kd = Kp * Td = 1.5

