#include <LoRa.h>
#include <ArduinoJson.h>

int counter = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("LoRa Sender");
  LoRa.setFrequency(433E6);

  LoRa.setPins(18, 14, 26);

  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}

void loop() {
  // Create a JSON document
  StaticJsonDocument<200> doc;
  doc["team1Score"] = 3;
  doc["team2Score"] = 0;

  // Serialize the JSON document to a string
  String jsonString;
  serializeJson(doc, jsonString);

  // Print the JSON string to Serial (optional)
  Serial.print("Sending JSON packet: ");
  Serial.println(jsonString);

  // send packet
  LoRa.beginPacket();
  LoRa.print(jsonString.c_str());
  LoRa.endPacket();

  delay(5000);
  
  
}
