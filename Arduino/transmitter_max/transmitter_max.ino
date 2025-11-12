// Transmitter Arduino
void setup() {
  Serial.begin(9600); // Start serial communication
  Serial.println("hello !!!");
}

void loop() {
  if (Serial.available() > 0) {
    String message = Serial.readStringUntil('\n'); // Read your input
    Serial.println(message); // Send it out via TX
    delay(200); // Small delay to stabilize transmission
  }
}
