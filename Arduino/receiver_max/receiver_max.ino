// Receiver Arduino
void setup() {
  Serial.begin(9600); // Start serial communication at 9600 baud
}

void loop() {
  if (Serial.available() > 0) { // Check if data is available
    String data = Serial.readStringUntil('\n'); // Read until newline
    Serial.println(data);
  }
}
