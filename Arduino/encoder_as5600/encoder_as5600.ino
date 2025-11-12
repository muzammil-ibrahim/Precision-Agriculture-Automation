#include <Wire.h>

#define AS5600_ADDR 0x36  // Default I2C address of AS5600
#define RAW_ANGLE_H 0x0C  // Register for raw angle high byte
#define RAW_ANGLE_L 0x0D  // Register for raw angle low byte

void setup() {
  Serial.begin(9600);
  Wire.begin();

  Serial.println("AS5600 Test Started...");
}

void loop() {
  uint16_t angle = readAngle();
  float degrees = (angle * 360.0) / 4096.0; // 12-bit output = 0–4095

  Serial.print("Raw angle: ");
  Serial.print(angle);
  Serial.print("\tDegrees: ");
  Serial.println(degrees, 2);

  delay(500);
}

uint16_t readAngle() {
  Wire.beginTransmission(AS5600_ADDR);
  Wire.write(RAW_ANGLE_H);
  Wire.endTransmission();

  Wire.requestFrom(AS5600_ADDR, 2);
  while (Wire.available() < 2);

  uint8_t highByte = Wire.read();
  uint8_t lowByte = Wire.read();
  uint16_t angle = ((highByte & 0x0F) << 8) | lowByte;

  return angle;
}
