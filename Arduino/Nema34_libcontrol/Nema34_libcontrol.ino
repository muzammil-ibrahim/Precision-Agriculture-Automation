#include <AccelStepper.h>

#define PUL_PIN 2
#define DIR_PIN 3
#define ENA_PIN 4

#define PEND_PIN 6  // In-position input (PEND+ connected here)

const int stepsPerRev = 3200;  // DIP switch setting

AccelStepper stepper(AccelStepper::DRIVER, PUL_PIN, DIR_PIN);

void setup() {
  pinMode(ENA_PIN, OUTPUT);
  digitalWrite(ENA_PIN, LOW); // Enable driver

  pinMode(PEND_PIN, INPUT_PULLUP);  // PEND active = LOW (depends on wiring)

  Serial.begin(115200);

  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(500);
}

// New function: move and check result
bool moveAndCheck(float angle, bool dir) {
  long steps = (long)(stepsPerRev * (angle / 360.0));
  if (!dir) steps = -steps;

  stepper.moveTo(stepper.currentPosition() + steps);
  stepper.runToPosition();

  // Check PEND feedback
  if (digitalRead(PEND_PIN) == LOW) {
    Serial.println("✅ Move completed successfully (in-position).");
    return true;
  } else {
    Serial.println("❌ Move finished, but in-position signal not received.");
    return false;
  }
}

void loop() {
  Serial.print("PEND state: ");
  Serial.println(digitalRead(PEND_PIN));
  delay(500);
  if (moveAndCheck(360, true)) {
    Serial.println("Task 1 OK");
  } else {
    Serial.println("Task 1 FAILED");
  }

  if (moveAndCheck(360, true)) {
    Serial.println("Task 2 OK");
  } else {
    Serial.println("Task 2 FAILED");
  }
    delay(2000);
}
