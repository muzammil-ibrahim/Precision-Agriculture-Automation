int pwmPin = 9;       // PWM output pin
int relayPin = 7;     // Relay control pin (FR120N)
int pwmValue = 70;    // Constant speed
int delayTime = 5000; // 5 seconds

void setup() {
  pinMode(pwmPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  Serial.begin(9600);

  // Start with relay LOW and PWM constant
  digitalWrite(relayPin, LOW);
  analogWrite(pwmPin, pwmValue);

  Serial.println("System started in FORWARD mode");
}

void loop() {
  // ---------- Forward Mode ----------
  Serial.println("FORWARD → Relay LOW");
  delay(delayTime);

  // Stop PWM before changing direction
  Serial.println("Stopping motor before reverse...");
  analogWrite(pwmPin, 0);
  delay(1000);

  // Switch to REVERSE
  digitalWrite(relayPin, HIGH);
  Serial.println("Relay HIGH → REVERSE ON");
  delay(1000);

  // Resume motor at constant speed
  analogWrite(pwmPin, pwmValue);
  Serial.println("Motor running in REVERSE at PWM 70");
  delay(delayTime);

  // Stop PWM before changing direction again
  Serial.println("Stopping motor before forward...");
  analogWrite(pwmPin, 0);
  delay(1000);

  // Switch back to FORWARD
  digitalWrite(relayPin, LOW);
  Serial.println("Relay LOW → FORWARD ON");
  delay(1000);

  // Resume motor at constant speed
  analogWrite(pwmPin, pwmValue);
  Serial.println("Motor running in FORWARD at PWM 70");
}
