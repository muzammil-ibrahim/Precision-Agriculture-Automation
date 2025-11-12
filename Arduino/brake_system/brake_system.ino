#include <Servo.h>

Servo servoLeft;
Servo servoRight;

int leftPin = 10;
int rightPin = 11;

int leftPos = 0;     
int rightPos = 0;    
int targetLeft = 0;
int targetRight = 0;

int baseLeft = 0;     // store initial positions
int baseRight = 0;

unsigned long lastUpdate = 0;
const int stepDelay = 15;  // ms between steps (controls speed)

void setup() {
  Serial.begin(9600);

  servoLeft.attach(leftPin);
  servoRight.attach(rightPin);

  // Assume servos start at their current position
  leftPos = servoLeft.read();
  rightPos = servoRight.read();
  targetLeft = leftPos;
  targetRight = rightPos;

  // Save the initial (base) positions
  baseLeft = leftPos;
  baseRight = rightPos;

  Serial.println("Enter +angle or -angle (e.g. +15 or -10): limited to ±20° from start");
}

void loop() {
  unsigned long currentMillis = millis();

  // Smooth servo movement
  if (currentMillis - lastUpdate >= stepDelay) {
    lastUpdate = currentMillis;

    bool moved = false;

    if (leftPos < targetLeft) { leftPos++; moved = true; }
    else if (leftPos > targetLeft) { leftPos--; moved = true; }

    if (rightPos < targetRight) { rightPos++; moved = true; }
    else if (rightPos > targetRight) { rightPos--; moved = true; }

    if (moved) {
      servoLeft.write(leftPos);
      servoRight.write(rightPos);
    }
  }

  // Handle serial input
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    int delta = input.toInt();
    if (delta == 0) return;

    Serial.print("Received delta: ");
    Serial.println(delta);

    // Compute temporary targets
    int tempLeft = leftPos + delta;
    int tempRight = rightPos - delta;

    // Apply ±20° limit from initial (base) position
    targetLeft = constrain(tempLeft, baseLeft - 20, baseLeft + 20);
    targetRight = constrain(tempRight, baseRight - 20, baseRight + 20);

    Serial.print("New targets → Left: ");
    Serial.print(targetLeft);
    Serial.print("°, Right: ");
    Serial.println(targetRight);
  }
}
