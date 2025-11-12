#include <SoftwareSerial.h>

// Define two serial ports for two MAX3232 modules
SoftwareSerial serial1(2, 3);  // RX, TX for MAX3232 #1
SoftwareSerial serial2(4, 5);  // RX, TX for MAX3232 #2

void setup() {
  // Initialize all serial ports
  Serial.begin(9600);   // Serial Monitor (USB)
  serial1.begin(9600);  // MAX3232 #1
  serial2.begin(9600);  // MAX3232 #2

  Serial.println("=== Dual MAX3232 Chat Started ===");
  Serial.println("Type a message to send to BOTH devices.");
  Serial.println("--------------------------------------");
}

void loop() {
  // 1️⃣ Send input from Serial Monitor to both MAX3232 devices
  if (Serial.available()) {
    char c = Serial.read();

    // Send to both devices
    serial1.write(c);
    serial2.write(c);

    // Echo to Serial Monitor
    Serial.write(c);
  }

  // 2️⃣ Read incoming data from MAX3232 #1
  // if (serial1.available()) {
  //   char c = serial1.read();
  //   Serial.print("[MAX3232 #1] ");
  //   Serial.write(c);
  // }

  // // 3️⃣ Read incoming data from MAX3232 #2
  // if (serial2.available()) {
  //   char c = serial2.read();
  //   Serial.print("[MAX3232 #2] ");
  //   Serial.write(c);
  // }
}
