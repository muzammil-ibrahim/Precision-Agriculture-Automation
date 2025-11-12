// Smooth Throttle Control for e-Scooter Controller (ESP32)
// Uses true DAC output for 1–4 V throttle signal

const int dacPin = 25;   // use GPIO25 or GPIO26 (only these are DAC pins)
int currentValue = 0;    // current DAC value
int targetValue = 0;     // target DAC value

// Tune these for smoother or faster acceleration
const int ACCEL_STEP = 1;     // change per step (0–255 range)
const int ACCEL_DELAY = 30;   // ms delay between steps
const int MIN_DAC = 51;       // ≈1 V (51/255 * 5 V)
const int MAX_DAC = 110;      // ≈4 V (204/255 * 5 V)

void setup() {
  Serial.begin(115200);
  Serial.println("Enter target speed (0–255):");
}

void loop() {
  // Read user input from Serial
  if (Serial.available()) {
    String dacInput = Serial.readStringUntil('\n');   // read till newline
    int input = dacInput.toInt();                  // convert to int

    input = constrain(input, 0, 255);

    // Map 0–255 input to 1–4 V DAC range
    targetValue = map(input, 0, 255, MIN_DAC, MAX_DAC);
    Serial.print("Target speed set to: ");
    Serial.println(targetValue);
  }

  // Smoothly ramp toward target
  if (currentValue < targetValue) {
    currentValue += ACCEL_STEP;
    if (currentValue > targetValue) currentValue = targetValue;
  } else if (currentValue > targetValue) {
    currentValue -= ACCEL_STEP;
    if (currentValue < targetValue) currentValue = targetValue;
  }

  // Write analog voltage via DAC
  dacWrite(dacPin, currentValue);

  delay(ACCEL_DELAY);
}
