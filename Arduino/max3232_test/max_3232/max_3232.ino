#include <SoftwareSerial.h>

// SoftwareSerial for MAX3232 connection
SoftwareSerial RS232(2, 3);  // RX, TX

int ID = 1;  // Change this to 2 on the second Arduino
String inputBuffer = "";
String incomingMessage = "";

void setup() {
  Serial.begin(9600);   // Serial Monitor
  RS232.begin(9600);    // MAX3232 communication
  Serial.print("Arduino ");
  Serial.print(ID);
  Serial.println(" ready. Type your message:");
}

void loop() {
  // 1️⃣ Read from Serial Monitor (your PC)
  if (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      // Send message with ID tag
      RS232.print(String(ID) + ":" + inputBuffer + "\n");
      inputBuffer = "";
    } else {
      inputBuffer += c;
    }
  }

  // 2️⃣ Read from other Arduino via MAX3232
  if (RS232.available()) {
    char c = RS232.read();
    if (c == '\n') {
      int separatorIndex = incomingMessage.indexOf(':');
      if (separatorIndex != -1) {
        int senderID = incomingMessage.substring(0, separatorIndex).toInt();
        String msg = incomingMessage.substring(separatorIndex + 1);

        // Print only if message came from the other Arduino
        if (senderID != ID) {
          Serial.print("Arduino ");
          Serial.print(senderID);
          Serial.print(": ");
          Serial.println(msg);
        }
      }
      incomingMessage = "";
    } else {
      incomingMessage += c;
    }
  }
}
