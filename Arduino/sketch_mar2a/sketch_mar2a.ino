void setup() {
  Serial.begin(9600);
}

void loop() {
  while(Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    Serial.println("Received: " + data);
  }
}
