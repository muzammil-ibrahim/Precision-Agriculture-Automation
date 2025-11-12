int pwmInputPin = 2;   // From Pixhawk RC output
int pwmOutputPin = 9;  // To motor/ESC or similar

void setup() {
  pinMode(pwmInputPin, INPUT);
  pinMode(pwmOutputPin, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  // Read input pulse width from Pixhawk (in microseconds)
  unsigned long pwmValue = pulseIn(pwmInputPin, HIGH, 25000); // timeout 25ms
  
  if (pwmValue > 900 && pwmValue < 2100) { // Valid RC range
    // Map 1000–2000 µs input to 0–255 PWM duty cycle
    int outputValue = map(pwmValue, 1000, 2000, 0, 255);
    
    analogWrite(pwmOutputPin, outputValue);
    
    Serial.print("Input PWM: ");
    Serial.print(pwmValue);
    Serial.print(" us, Output Duty: ");
    Serial.println(outputValue);
  } else {
    // No valid signal detected — stop output
    analogWrite(pwmOutputPin, 0);
  }
}
