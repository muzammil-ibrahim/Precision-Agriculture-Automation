#include <Arduino.h>
#define DE_RE_PIN 4

uint8_t packet_move[10];
const uint8_t packet_length = 10;

// --- CRC8/MAXIM helper (same as your working code) ---
uint8_t crc8_update(uint8_t crc, uint8_t data) {
  crc ^= data;
  for (uint8_t i = 0; i < 8; i++)
    crc = (crc & 1) ? (crc >> 1) ^ 0x8C : (crc >> 1);
  return crc;
}

// --- Send frame over RS485 (toggle DE/RE) ---
void sendFrame(uint8_t *packet) {
  digitalWrite(DE_RE_PIN, HIGH);   // Enable transmit
  delayMicroseconds(200);
  Serial1.write(packet, packet_length);
  Serial1.flush();
  digitalWrite(DE_RE_PIN, LOW);    // Back to receive
  delayMicroseconds(200);
}

// --- Change mode (keeps your working implementation) ---
void ddsm_change_mode(uint8_t id, uint8_t mode) {
  packet_move[0] = id;
  packet_move[1] = 0xA0;
  for (int i = 2; i < 9; i++) packet_move[i] = 0x00;
  packet_move[9] = mode; // your working code set mode directly in last byte

  sendFrame(packet_move);
  Serial.print("Mode change packet: ");
  for (uint8_t i = 0; i < packet_length; i++) {
    Serial.print(packet_move[i], HEX); Serial.print(" ");
  }
  Serial.println();
}

// --- Position control (builds packet and CRC as in your working code) ---
void ddsm_ctrl(uint8_t id, int cmd, uint8_t act) {
  packet_move[0] = id;
  packet_move[1] = 0x64;

  packet_move[2] = (cmd >> 8) & 0xFF;
  packet_move[3] = cmd & 0xFF;
  packet_move[4] = 0x00;
  packet_move[5] = 0x00;
  packet_move[6] = act;
  packet_move[7] = 0x00;

  uint8_t crc = 0;
  for (size_t i = 0; i < packet_length - 1; ++i) {
    crc = crc8_update(crc, packet_move[i]);
  }
  packet_move[9] = crc;

  sendFrame(packet_move);
  Serial.print("Target position (counts): "); Serial.println(cmd);
}

// --- Convert degrees to counts (same mapping as your working code) ---
// maps [-180, 180] -> [0, 32767], with 0° -> 16384 counts
int degToCounts(float deg) {
  int counts = (int)(deg * 32767.0 / 360.0) + 16384;
  if (counts < 0) counts = 0;
  if (counts > 32767) counts = 32767;
  return counts;
}

// --- Convert counts back to degrees (inverse of degToCounts) ---
float countsToDeg(int counts) {
  return (counts - 16384) * 360.0f / 32767.0f;
}

// --- Query motor mode (kept from reference) ---
void query_mode(uint8_t id) {
  uint8_t query[10] = {id, 0x74, 0,0,0,0,0,0,0,0x04};
  sendFrame(query);
  Serial.println("Query mode packet sent");
}

// --- Read & parse feedback frame (wait up to timeoutMs) ---
bool readAndPrintFeedback(unsigned long timeoutMs) {
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (Serial1.available() >= 10) {
      uint8_t buf[10];
      Serial1.readBytes(buf, 10);

      // verify CRC (compute over bytes 0..8)
      uint8_t crc = 0;
      for (int i = 0; i < 9; ++i) crc = crc8_update(crc, buf[i]);

      if (crc != buf[9]) {
        Serial.print("CRC mismatch! got: 0x");
        Serial.print(buf[9], HEX);
        Serial.print(" calc: 0x");
        Serial.println(crc, HEX);
        // attempt to continue / discard this frame
        return false;
      }

      // parse feedback fields (as used before)
      uint8_t id = buf[0];
      uint8_t cmd = buf[1];

      uint16_t counts = ((uint16_t)buf[2] << 8) | buf[3]; // 0..32767
      int16_t velocity = (int16_t)(((uint16_t)buf[4] << 8) | buf[5]); // signed
      int16_t current  = (int16_t)(((uint16_t)buf[6] << 8) | buf[7]); // signed
      uint8_t temp = buf[8];

      float deg = countsToDeg(counts);

      Serial.println("--- Feedback Frame ---");
      Serial.print("ID: "); Serial.println(id, DEC);
      Serial.print("CMD: 0x"); Serial.println(cmd, HEX);
      Serial.print("Counts: "); Serial.println(counts);
      Serial.print("Angle (deg): "); Serial.println(deg, 2);
      Serial.print("Velocity: "); Serial.println(velocity);
      Serial.print("Current: "); Serial.println(current);
      Serial.print("Temp: "); Serial.println(temp);
      Serial.println("----------------------");
      return true;
    }
    delay(5);
  }
  Serial.println("Feedback timeout");
  return false;
}

void setup() {
  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);

  Serial.begin(115200);
  Serial1.begin(115200);
  delay(500);

  Serial.println("DDSM115: Position Mode Move + Feedback Example");

  // 1. Switch to POSITION mode (0x03 per your working example)
  ddsm_change_mode(0x00, 0x03);
  delay(300);

  // 2. Query/confirm mode
  query_mode(0x00);
  delay(300);

  // Movement sequence (I kept your earlier sequence; degToCounts clamps out-of-range)
  int targets[] = {0, 90, 180};
  const int nTargets = sizeof(targets) / sizeof(targets[0]);

  for (int i = 0; i < nTargets; ++i) {
    int counts = degToCounts((float)targets[i]);
    ddsm_ctrl(0x00, counts, 0x01); // act=0x01 as in your working example

    // wait a short time for motor to respond then read feedback (up to 1500 ms)
    delay(1500);
    bool ok = readAndPrintFeedback(1500);
    if (!ok) {
      Serial.println("No valid feedback after command; continuing...");
    }

    // wait longer for motion to finish (adjust as needed)
    delay(2000);
  }

  Serial.println("Movement sequence complete.");
}

void loop() {
  // Nothing here. If you want continuous cycling, place code here.
}
