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
  // map [-180, 180] degrees to [0, 32767]
  // 0° → 16384 counts (middle point)
  int counts = (int)(deg * 32767.0 / 360.0) + 16384;
  if (counts < 0) counts = 0;
  if (counts > 32767) counts = 32767;
  return counts;
}

// --- Query motor mode ---
void query_mode(uint8_t id) {
  uint8_t query[10] = {id, 0x74, 0,0,0,0,0,0,0,0x04};
  sendFrame(query);
  Serial.println("Query mode packet sent");
}

void setup() {
  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);

  Serial.begin(115200);
  Serial1.begin(115200);
  delay(500);

  Serial.println("DDSM115: Position Mode Move Example");

  // 1. Switch to position mode
  ddsm_change_mode(0x00, 0x03);
  delay(500);

  // 2. Confirm mode
  query_mode(0x00);
  delay(500);

  // --- Movement sequence ---
  ddsm_ctrl(0x00, degToCounts(90), 0x01);   // +90°
  delay(2000);

  ddsm_ctrl(0x00, degToCounts(-90), 0x01);  // -90°
  delay(2000);

  ddsm_ctrl(0x00, degToCounts(45), 0x01);   // +45°
  delay(2000);

  ddsm_ctrl(0x00, degToCounts(-45), 0x01);  // -45°
  delay(2000);

  Serial.println("Movement sequence complete.");
}

void loop() {
  // Do nothing, or keep querying mode if you like
}
