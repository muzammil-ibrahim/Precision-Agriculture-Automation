#include <Arduino.h>
#define DE_RE_PIN 4

uint8_t crc8_maxim(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++) {
      crc = (crc & 1) ? (crc >> 1) ^ 0x8C : (crc >> 1);
    }
  }
  return crc;
}

void sendFrame(const uint8_t *frame9) {
  digitalWrite(DE_RE_PIN, HIGH); delayMicroseconds(200);
  Serial1.write(frame9, 9);
  uint8_t crc = crc8_maxim(frame9, 9);
  Serial1.write(crc);
  Serial1.flush();
  delayMicroseconds(200);
  digitalWrite(DE_RE_PIN, LOW);
}

void setup() {
  pinMode(DE_RE_PIN, OUTPUT); digitalWrite(DE_RE_PIN, LOW);
  Serial.begin(115200); Serial1.begin(115200);
  Serial.println("Running DDSM115: 50 RPM for 10s, then brake");
  delay(500);

  // 1. Switch to velocity mode
  uint8_t modeCmd[9] = {0x00, 0xA0, 0,0,0,0,0,0, 0x02};
  sendFrame(modeCmd);
  delay(200);

  // 2. Send 50 RPM command
  uint8_t velCmd[9] = {0x00, 0x64, 0x00, 0x32, 0,0,0,0, 0x00};
  sendFrame(velCmd);
  Serial.println("Motor running at 50 RPM");
  
  delay(5000);  // Wait 10 seconds

    // 3. Brake/stop
  uint8_t brakeCmd1[9] = {0x00, 0x64, 0x00, 0x00, 0,0,0, 0xFF, 0x00};
  sendFrame(brakeCmd1);
  Serial.println("Brake command sent");
  delay(2000);

  // // Command frame for 100 RPM
  // uint8_t velCmd1[10] = { 
  // 0x01, // ID
  // 0x64, // Command: velocity loop
  // 0x00, 0x64, // Velocity = 100 (0x0064)
  // 0x00, 0x00, 0x00, 0x00, // Reserved
  // 0x4F  // CRC
  // };

  // sendFrame(velCmd1);
  // Serial.println("Motor running at 100 RPM");
  
  // delay(5000);

  // // 3. Brake/stop
  // uint8_t brakeCmd[9] = {0x01, 0x64, 0x00, 0x00, 0,0,0, 0xFF, 0x00};
  // sendFrame(brakeCmd);
  // Serial.println("Brake command sent");
}

void loop() {
  // Done
}