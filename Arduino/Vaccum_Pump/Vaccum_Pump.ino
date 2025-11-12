// Kamoer HLVP15-AU24 pump automatic 2s ON / 2s OFF control
// PWM output on pin 9 -> Pump White/Blue
// Pump Red -> +24V, Pump Black -> GND (common with Arduino)

const int pwmPin = 9;
const int pumpSpeed = 100; // desired speed percentage (0–100)

void setupPWM20kHz() {
  pinMode(pwmPin, OUTPUT);
  // Configure Timer1 for 20kHz Fast PWM on pin 9
  TCCR1A = _BV(COM1A1) | _BV(WGM11);
  TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10);
  ICR1 = 799;  // sets 20kHz frequency (16MHz / (1*(799+1)))
  OCR1A = 0;   // start with 0% duty (pump off)
}

void setDuty(uint8_t duty) {
  if (duty > 100) duty = 0;
  OCR1A = (uint16_t)((uint32_t)duty * (ICR1 + 1) / 100);
}

void setup() {
  setupPWM20kHz();
}

void loop() {
  // Turn pump ON
  setDuty(pumpSpeed);
  delay(2000);  // run for 2 seconds

  // Turn pump OFF
  setDuty(0);
  delay(2000);  // stop for 2 seconds
}
