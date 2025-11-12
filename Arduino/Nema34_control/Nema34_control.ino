#define PUL_PIN 2
#define DIR_PIN 3
#define ENA_PIN 4

const long stepsPerRev = 3200; // from DIP switch setting

void setup() {
  pinMode(PUL_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);

  digitalWrite(ENA_PIN, LOW); // enable driver
}

// Function to move motor by angle
void moveByAngle(float angle, bool dir) {
  long totalSteps = (long)(stepsPerRev * (angle / 360.0));

  digitalWrite(DIR_PIN, dir ? HIGH : LOW);

  for (long i = 0; i < totalSteps; i++) {
    digitalWrite(PUL_PIN, HIGH);
    delayMicroseconds(500); // speed control (smaller = faster)
    digitalWrite(PUL_PIN, LOW);
    delayMicroseconds(500);
  }
}

void loop() {
  moveByAngle(90, true);   // Rotate +90°
  delay(1000);

  moveByAngle(90, false); // Rotate -180°
  delay(2000);
}
