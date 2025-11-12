#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <AccelStepper.h>

// ================= HUB MOTOR SETUP =================
Adafruit_MCP4725 dac;

int relayPin = 7;
int pwmValue = 70;

float dacVoltage = 0;
int dacValue = 0;
bool isForwardHub = true; // default direction = forward

// ================= STEPPER MOTOR SETUP =================
#define PUL_PIN 2
#define DIR_PIN 3
#define ENA_PIN 4
#define PEND_PIN 6
#define stepsPerRev 3200  // your driver’s DIP switch setting

AccelStepper stepper(AccelStepper::DRIVER, PUL_PIN, DIR_PIN);

// ================= UNIQUE DEVICE ID =================
String DEVICE_ID = "ID1";  // <-- Change this for each Arduino (e.g. ID2, ID3, ID4)

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  Wire.begin();

  dac.begin(0x60);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  dac.setVoltage(0, false);

  pinMode(ENA_PIN, OUTPUT);
  digitalWrite(ENA_PIN, LOW);
  pinMode(PEND_PIN, INPUT_PULLUP);

  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(500);

  Serial.println(F("=== Combined Hub Motor + Stepper Control ==="));
  Serial.print(F("Device ID: "));
  Serial.println(DEVICE_ID);
  Serial.println(F("Commands:"));
  Serial.println(F("  H <ID> <voltage> [F/R]"));
  Serial.println(F("  S <ID> <angle> [CW/CCW]"));
  Serial.println(F("------------------------------------------"));
}

// ================= HUB MOTOR CONTROL =================
void controlHubMotor(float voltage, char dirChar) {
  if (dirChar == 'F') isForwardHub = true;
  else if (dirChar == 'R') isForwardHub = false;

  // Switch relay first
  digitalWrite(relayPin, isForwardHub ? LOW : HIGH);
  delay(50); 

  // Then apply DAC voltage
  voltage = constrain(voltage, 0.0, 5.0);
  dacValue = (int)((voltage / 5.0) * 4095);
  dac.setVoltage(dacValue, false);
  dacVoltage = voltage;

  Serial.print(F("Hub Motor → Direction: "));
  Serial.print(isForwardHub ? F("FORWARD") : F("REVERSE"));
  Serial.print(F(" | DAC: "));
  Serial.print(dacValue);
  Serial.print(F(" | Voltage ≈ "));
  Serial.print(voltage, 2);
  Serial.println(F(" V"));
}



// ================= STEPPER CONTROL =================
bool moveStepper(float angle, bool cw) {
  long steps = (long)(stepsPerRev * (angle / 360.0));
  if (!cw) steps = -steps;

  stepper.moveTo(stepper.currentPosition() + steps);
  stepper.runToPosition();

  if (digitalRead(PEND_PIN) == LOW) {
    Serial.println(F("✅ Stepper move completed (in-position)."));
    return true;
  } else {
    Serial.println(F("⚠️ Stepper move finished, but in-position not detected."));
    return false;
  }
}

// ================= MAIN LOOP =================
void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() == 0) return;

    char command = toupper(input.charAt(0));
    input.remove(0, 1);  // remove command character
    input.trim();

    int space1 = input.indexOf(' ');
    if (space1 == -1) {
      Serial.println(F("⚠️ Invalid format. Missing ID."));
      return;
    }

    String id = input.substring(0, space1);
    input.remove(0, space1 + 1);
    input.trim();

    // ===== ID CHECK =====
    if (id != DEVICE_ID) {
      // Ignore commands for other devices
      return;
    }

    if (command == 'H') {
      // Format: H <ID> <voltage> [F/R]
      float voltage = 0.0;
      char direction = '\0';
      int space2 = input.indexOf(' ');
      if (space2 == -1) {
        voltage = input.toFloat();
      } else {
        voltage = input.substring(0, space2).toFloat();
        direction = toupper(input.charAt(space2 + 1));
      }
      controlHubMotor(voltage, direction);

    } else if (command == 'S') {
      // Format: S <ID> <angle> [CW/CCW]
      float angle = 0.0;
      String dirStr = "";
      int space2 = input.indexOf(' ');
      if (space2 == -1) {
        angle = input.toFloat();
        dirStr = "CW";
      } else {
        angle = input.substring(0, space2).toFloat();
        dirStr = input.substring(space2 + 1);
        dirStr.toUpperCase();
      }

      bool cw = (dirStr != "CCW");
      moveStepper(angle, cw);

    } else {
      Serial.println(F("⚠️ Unknown command. Use H or S."));
    }
  }
}

