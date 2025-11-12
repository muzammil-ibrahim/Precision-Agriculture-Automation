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
  Serial.print("Mode change packet: ");
  for (uint8_t i = 0; i < packet_length; i++) {
    Serial.print(packet_move[i], HEX); Serial.print(" ");
  }
  Serial.println();
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
  Serial.print("Target position (counts): "); Serial.println(cmd);
}

// --- Convert degrees to counts ---
int degToCounts(float deg) {
  int counts = (int)(deg * 32767.0 / 360.0) + 16384;
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
  Serial.println("Feedback query sent");
}

// --- Read feedback and extract angle ---
void read_feedback() {
  if (Serial1.available() >= 10) {
    uint8_t data[10];
    Serial1.readBytes(data, 10);

    // --- CRC check ---
    uint8_t crc = 0;
    for (int i = 0; i < 9; i++) crc = crc8_update(crc, data[i]);

    if (crc != data[9]) {
      Serial.println("⚠️ CRC mismatch!");
      return;
    }

    // Parse fields
    uint8_t id      = data[0];
    uint8_t mode    = data[1];
    int16_t torque  = (data[2] << 8) | data[3];
    int16_t velocity= (data[4] << 8) | data[5];
    uint8_t temp    = data[6];
    uint8_t pos_u8  = data[7];   // 0–255 → 0–360°
    uint8_t err     = data[8];

    float angle = (pos_u8 / 255.0) * 360.0;

    // Print results
    Serial.print("ID: "); Serial.print(id);
    Serial.print(" | Mode: "); Serial.print(mode, HEX);
    Serial.print(" | Torque: "); Serial.print(torque);
    Serial.print(" | Velocity: "); Serial.print(velocity);
    Serial.print(" | Temp: "); Serial.print(temp);
    Serial.print("°C | Angle: "); Serial.print(angle, 1);
    Serial.print("° | Err: 0x"); Serial.println(err, HEX);
  }
}

void setup() {
  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);

  Serial.begin(115200);
  Serial1.begin(115200);
  delay(500);

  Serial.println("DDSM115: Position Mode Move Example");

  // 1. Switch to position mode
  ddsm_change_mode(0x01, 0x03);
  delay(500);


  // 2. Confirm mode
  query_feedback(0x01);
  delay(500);
  read_feedback();
  delay(500);

  // --- Movement sequence ---
  ddsm_ctrl(0x01, degToCounts(-45), 0x01);   // +90°
  delay(2000);
  query_feedback(0x01);   // ask for feedback
  delay(200);
  read_feedback();
}

void loop() {
  // Optionally keep polling feedback
  query_feedback(0x01);
  delay(500);
  read_feedback();
  
}
