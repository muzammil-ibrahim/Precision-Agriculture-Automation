// Ball Screw Actuator Calibration (for HSD86 Hybrid Servo Driver)
// Two endstops: home and far end. Measures full travel in steps and stores steps/mm, max travel, offsets.

#include <EEPROM.h>

// ------------- USER CONFIG ----------------
const int STEP_PIN    = 2;
const int DIR_PIN     = 3;
const int ENABLE_PIN  = 4; // Active LOW (HSD86 ENA+)
const int HOME_PIN    = 9; // Near end limit (active LOW)
const int END_PIN     = 10; // Far end limit (active LOW)

const unsigned long STEP_PULSE_US = 500; // adjust to control speed (lower = faster)
const int MICROSTEPS = 3200;   // your HSD86 DIP switch microstep setting
const int MOTOR_FULL_STEPS = 200; // typical 1.8° stepper (for info only)
const float SCREW_LEAD_MM = 8.0;  // mm per screw revolution (check your actuator)

const int EEPROM_ADDR = 0;
// -------------------------------------------

struct CalData {
  uint16_t version;
  float steps_per_mm;
  float home_offset_mm;
  float max_travel_mm;
  uint16_t crc16;
};

const uint16_t CAL_VERSION = 0xBEEF;

// Simple CRC16
uint16_t simple_crc16(const uint8_t *data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    crc = (crc << 1) | (crc >> 15);
  }
  return crc;
}

void enableDriver(bool enable) {
  digitalWrite(ENABLE_PIN, enable ? LOW : HIGH); // active LOW
}

void stepPulse() {
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(STEP_PULSE_US);
  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(STEP_PULSE_US);
}

bool isHomePressed() { return digitalRead(HOME_PIN) == HIGH; }
bool isEndPressed() { return digitalRead(END_PIN) == HIGH; }

void moveUntilLimit(bool dirPositive, int limitPin) {
  digitalWrite(DIR_PIN, dirPositive ? HIGH : LOW);
  while (digitalRead(limitPin) != LOW) {
    stepPulse();
  }
}

void moveSteps(long steps, bool dirPositive) {
  digitalWrite(DIR_PIN, dirPositive ? HIGH : LOW);
  for (long i = 0; i < steps; i++) {
    stepPulse();
  }
}

void doHome() {
  enableDriver(true);
  Serial.println(F("Homing..."));
  // If already pressed, back off first
  if (isHomePressed()) {
    moveSteps(800, true);
  }
  // Move toward home
  digitalWrite(DIR_PIN, LOW);
  while (!isHomePressed()) {
    stepPulse();
  }
  Serial.println(F("Home reached."));
  // Back off and approach again slowly
  moveSteps(400, true);
  delay(50);
  digitalWrite(DIR_PIN, LOW);
  while (!isHomePressed()) {
    stepPulse();
    delayMicroseconds(STEP_PULSE_US * 2);
  }
  enableDriver(false);
  Serial.println(F("Home set."));
}

void writeCal(const CalData &c) {
  CalData temp = c;
  temp.crc16 = 0;
  temp.crc16 = simple_crc16((uint8_t *)&temp, sizeof(CalData));
  EEPROM.put(EEPROM_ADDR, temp);
}

bool readCal(CalData &out) {
  EEPROM.get(EEPROM_ADDR, out);
  if (out.version != CAL_VERSION) return false;
  uint16_t stored = out.crc16;
  out.crc16 = 0;
  uint16_t calc = simple_crc16((uint8_t *)&out, sizeof(CalData));
  return (stored == calc);
}

void printCal() {
  CalData c;
  if (readCal(c)) {
    Serial.println(F("== Calibration Data =="));
    Serial.print(F("Steps per mm: ")); Serial.println(c.steps_per_mm, 6);
    Serial.print(F("Max travel mm: ")); Serial.println(c.max_travel_mm, 3);
    Serial.print(F("Home offset mm: ")); Serial.println(c.home_offset_mm, 3);
  } else {
    Serial.println(F("No valid calibration in EEPROM."));
  }
}

// ---- AUTO CALIBRATION USING BOTH LIMITS ----
void autoCalibrate() {
  enableDriver(true);
  Serial.println(F("Starting full-travel calibration..."));
  Serial.println(F("Homing first..."));
  doHome();
  delay(500);

  Serial.println(F("Moving toward END limit..."));
  long stepCount = 0;
  enableDriver(true);
  digitalWrite(DIR_PIN, HIGH);
  while (!isEndPressed()) {
    stepPulse();
    stepCount++;
    if (stepCount % 5000 == 0) Serial.print(".");
  }
  Serial.println();
  Serial.print(F("Reached END switch after steps: "));
  Serial.println(stepCount);

  enableDriver(false);

  while (Serial.available()) Serial.read();  // clear leftover input
  Serial.println(F("Measure the actual physical travel distance between the two switches (mm):"));
  Serial.print("> ");
  while (!Serial.available()) { delay(50); } // now wait for fresh input
  float measured_mm = Serial.parseFloat();

  if (measured_mm <= 0) {
    Serial.println(F("Invalid input. Aborting."));
    return;
  }

  float steps_per_mm = (float)stepCount / measured_mm;

  CalData c;
  c.version = CAL_VERSION;
  c.steps_per_mm = steps_per_mm;
  c.home_offset_mm = 0.0;
  c.max_travel_mm = measured_mm;
  writeCal(c);

  Serial.println(F("\nCalibration complete!"));
  Serial.print(F("steps_per_mm = ")); Serial.println(steps_per_mm, 6);
  Serial.print(F("max_travel_mm = ")); Serial.println(measured_mm, 3);
  Serial.println(F("Data saved to EEPROM."));
}

void setup() {
  Serial.begin(115200);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(HOME_PIN, INPUT_PULLUP);
  pinMode(END_PIN, INPUT_PULLUP);
  enableDriver(false);

  Serial.println(F("\nHSD86 Ball Screw Auto-Calibrator"));
  printCal();

  Serial.println(F("\nCommands:"));
  Serial.println(F("  h -> Home"));
  Serial.println(F("  c -> Auto Calibrate (home + measure to end)"));
  Serial.println(F("  p -> Print calibration data"));
}

void loop() {
  if (!Serial.available()) return;
  char cmd = Serial.read();
  switch (cmd) {
    case 'h': doHome(); break;
    case 'c': autoCalibrate(); break;
    case 'p': printCal(); break;
    default: break;
  }
}
