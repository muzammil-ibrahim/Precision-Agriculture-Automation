int pwmPin = 9;
int pwmValue = 0;

void setup() {
  pinMode(pwmPin, OUTPUT);
  Serial.begin(9600);
  Serial.println("Enter PWM value (0 - 255):");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    pwmValue = input.toInt();
    pwmValue = constrain(pwmValue, 0, 255);
    analogWrite(pwmPin, pwmValue);
    
    // 66 min value dead zone
    Serial.print("Applied PWM value: ");
    Serial.println(pwmValue);
    Serial.println("Enter next PWM value (0 - 255):");
  }
}
