#include <Arduino.h>
#define DE_RE_PIN 4

uint8_t packet_move[10];
const uint8_t packet_length = 10;

// --- CRC8/MAXIM helper ---
uint8_t crc8_update(uint8_t crc, uint8_t data) {
  crc ^= data;
  for (uint8_t i = 0; i < 8; i++)
    crc = (crc & 1) ? (crc >> 1) ^ 0x8C : (crc >> 1);
  return crc;
}

// --- Send frame ---
void sendFrame(uint8_t *packet) {
  digitalWrite(DE_RE_PIN, HIGH);   // Enable transmit
  delayMicroseconds(200);
  Serial1.write(packet, packet_length);
  Serial1.flush();
  digitalWrite(DE_RE_PIN, LOW);    // Back to receive
  delayMicroseconds(200);
}

// --- Change mode ---
void ddsm_change_mode(uint8_t id, uint8_t mode) {
  packet_move[0] = id;
  packet_move[1] = 0xA0;
  for (int i = 2; i < 9; i++) packet_move[i] = 0x00;
  packet_move[9] = mode;

  sendFrame(packet_move);
}

// --- Position control ---
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
}

// --- Convert degrees to counts ---
int degToCounts(float deg) {
  int counts = (int)(deg * 32767.0 / 360.0);
  if (counts < 0) counts = 0;
  if (counts > 32767) counts = 32767;
  return counts;
}

// --- Query motor feedback (Protocol 2) ---
void query_feedback(uint8_t id) {
  uint8_t query[10] = {id, 0x74, 0,0,0,0,0,0,0,0};
  uint8_t crc = 0;
  for (int i = 0; i < 9; i++) crc = crc8_update(crc, query[i]);
  query[9] = crc;
  sendFrame(query);
}

// // --- Read feedback and print only angle ---
// void read_feedback() {
//   if (Serial1.available() >= 10) {
//     uint8_t data[10];
//     Serial1.readBytes(data, 10);

//     // CRC check
//     uint8_t crc = 0;
//     for (int i = 0; i < 9; i++) crc = crc8_update(crc, data[i]);
//     if (crc != data[9]) {
//       return; // bad packet
//     }

//     // Extract angle (DATA[7] → 0-255 → 0-360°)
//     uint8_t pos_u8 = data[7];
//     float angle = (pos_u8 / 255.0) * 360.0;

//     // Print only angle
//     Serial.print("ANGLE:");
//     Serial.println(angle, 1);
//   }
// }


float zero_offset = -1;  // invalid initially

void read_feedback() {
  if (Serial1.available() >= 10) {
    uint8_t data[10];
    Serial1.readBytes(data, 10);

    // CRC check
    uint8_t crc = 0;
    for (int i = 0; i < 9; i++) crc = crc8_update(crc, data[i]);
    if (crc != data[9]) return;

    // Extract raw angle
    uint8_t pos_u8 = data[7];
    float angle = (pos_u8 / 255.0) * 360.0;

    // Software zero
    if (zero_offset < 0) {
      zero_offset = angle;  // store first angle
    }

    float relative_angle = angle - zero_offset;
    if (relative_angle < 0) relative_angle += 360.0;

    // Debug: print both
    // Serial.print("RAW: ");
    // Serial.print(angle, 1);
    Serial.print("ANGLE: ");
    Serial.println(relative_angle, 1);
  }
}



void setup() {
  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);

  Serial.begin(115200);
  Serial1.begin(115200);
  delay(500);

  // 1. Switch to position mode
  ddsm_change_mode(0x01, 0x03);
  delay(500);

  // 2. Initial feedback
  query_feedback(0x01);
  delay(200);
  read_feedback();

  // --- Example move ---
  // ddsm_ctrl(0x01, degToCounts(90), 0x01);   // Move +90°
  // delay(2000);
  // query_feedback(0x01);
  // delay(200);
  // read_feedback();
}

void loop() {

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    if (cmd.startsWith("MOVE:")) {
      float targetAngle = cmd.substring(5).toFloat();
      int counts = degToCounts(targetAngle);
      ddsm_ctrl(0x01, counts, 0x01);
    }
  }

  query_feedback(0x01);
  delay(500);
  read_feedback();
}
