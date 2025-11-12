// THE RIGHT MOTOR WHEN + MOVES  IN CLOCKWISE AND FOR - ANTI CLOCKWISE (11)
// THE LEFT MOTOR WHEN + MOVES  IN ANTICLOCKWISE AND FOR --10 CLOCKWISE (10)

#include <Servo.h>

Servo servoLeft;
Servo servoRight;

int leftPin = 10;
int rightPin = 11;

int leftPos = 0;     // will sync with current position
int rightPos = 0;    // will sync with current position
int targetLeft = 0;
int targetRight = 0;

unsigned long lastUpdate = 0;
const int stepDelay = 15;  // speed control (ms between steps)

void setup() {
  Serial.begin(9600);
  //Serial.println("Enter +angle or -angle (e.g. +60 or -45):");

  servoLeft.attach(leftPin);
  servoRight.attach(rightPin);

  // Do NOT move — assume current position is where servos are physically
  leftPos = servoLeft.read();
  rightPos = servoRight.read();
  targetLeft = leftPos;
  targetRight = rightPos;
}

void loop() {
  unsigned long currentMillis = millis();

  // Update both servos smoothly and in sync
  if (currentMillis - lastUpdate >= stepDelay) {
    lastUpdate = currentMillis;

    bool moved = false;

    if (leftPos < targetLeft) { leftPos++; moved = true; }
    else if (leftPos > targetLeft) { leftPos--; moved = true; }

    if (rightPos < targetRight) { rightPos++; moved = true; }
    else if (rightPos > targetRight) { rightPos--; moved = true; }

    if (moved) {
      // Update both at same instant → visually synchronous
      servoLeft.write(leftPos);
      servoRight.write(rightPos);
    }
  }

  // Handle serial input (non-blocking)
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    int delta = input.toInt();
    if (delta == 0) return;

    Serial.print("Received delta: ");
    Serial.println(delta);

    // Compute new relative target angles
    targetLeft = constrain(leftPos + delta, 0, 180);
    targetRight = constrain(rightPos - delta, 0, 180);

    Serial.print("New targets → Left: ");
    Serial.print(targetLeft);
    Serial.print("°, Right: ");
    Serial.println(targetRight);
  }
}
