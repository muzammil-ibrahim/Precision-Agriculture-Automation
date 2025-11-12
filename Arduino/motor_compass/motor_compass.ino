#include <AccelStepper.h>
#include <QMC5883L.h>
#include <Wire.h>

// -------------------- Stepper Motor Pins --------------------
#define PUL_PIN 2
#define DIR_PIN 3
#define ENA_PIN 4

// -------------------- Stepper Configuration --------------------
const int stepsPerRev = 3200;  // Adjust per DIP switch setting
AccelStepper stepper(AccelStepper::DRIVER, PUL_PIN, DIR_PIN);

// -------------------- Compass Setup --------------------
QMC5883L compass;
int heading = 0;

// -------------------- Setup --------------------
void setup() {
  // Stepper setup
  pinMode(ENA_PIN, OUTPUT);
  digitalWrite(ENA_PIN, LOW); // Enable driver

  Serial.begin(115200);
  Serial.println("System Starting...");

  stepper.setMaxSpeed(100);
  stepper.setAcceleration(500);

  // Compass setup
  Wire.begin();
  compass.init();
  compass.setSamplingRate(50);

  Serial.println("QMC5883L Compass Initialized.");
  Serial.println("Turn compass in all directions to calibrate....");
}

// -------------------- Function: Move Motor --------------------
void moveMotor(float angle, bool dir) {
  long steps = (long)(stepsPerRev * (angle / 360.0));
  if (!dir) steps = -steps;

  stepper.moveTo(stepper.currentPosition() + steps);
  stepper.runToPosition();
}

// -------------------- Main Loop --------------------
void loop() {
  // --- Read Compass Angle ---
  heading = compass.readHeading();
  if (heading != 0) {
    Serial.print("Compass Heading: ");
    Serial.println(heading);
  } else {
    Serial.println("Compass Calibrating...");
  }

  // --- Stepper Movement +90° ---
  Serial.println("Rotating +90 degrees...");
  moveMotor(90, true);

  heading = compass.readHeading();
  if (heading != 0) {
    Serial.print("Heading after +90° move: ");
    Serial.println(heading);
  }

  delay(2000);

  // --- Stepper Movement -90° ---
  Serial.println("Rotating -90 degrees...");
  moveMotor(90, false);

  heading = compass.readHeading();
  if (heading != 0) {
    Serial.print("Heading after -90° move: ");
    Serial.println(heading);
  }

  delay(2000);
}
