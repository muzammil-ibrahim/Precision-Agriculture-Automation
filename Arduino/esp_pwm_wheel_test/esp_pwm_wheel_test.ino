const int dacPin = 25;

void setup() {
  Serial.begin(115200);
  Serial.println("Enter DAC value (0–255):");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');   // read till newline
    int dacValue = input.toInt();                  // convert to int
    dacValue = constrain(dacValue, 0, 255);
    dacWrite(dacPin, dacValue);

    Serial.print("Set DAC value: ");
    Serial.print(dacValue);
    Serial.print(" => Voltage ≈ ");
    Serial.print((dacValue / 255.0) * 3.3, 2);
    Serial.println(" V");
  }
}
