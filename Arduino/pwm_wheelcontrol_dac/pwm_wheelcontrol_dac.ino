// Desired Voltage	DAC Value (≈)
// 1 V	900
// 2 V	1638
// 3 V	2457
// 4 V	3276

// | Desired Voltage (Vout) | DAC Value (≈) |
// | ---------------------- | ------------- |
// | **1.2 V**              | **983**       |
// | **1.3 V**              | **1065**      |
// | **1.4 V**              | **1147**      |
// | **1.5 V**              | **1228**      |



#include <Wire.h>
#include <Adafruit_MCP4725.h>

Adafruit_MCP4725 dac;

void setup() {
  Serial.begin(115200);
  dac.begin(0x60); // Default I2C address of MCP4725
  dac.setVoltage(0,false);
  Serial.println("Enter value (0–4095):");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');   // read till newline
    int value = input.toInt();                  // convert to int
    value = constrain(value, 0, 4095);
    dac.setVoltage(value, false);

    float voltage = (value / 4095.0) * 5.0; // assuming 5V VCC
    Serial.print("DAC value: ");
    Serial.print(value);
    Serial.print(" => Voltage ≈ ");
    Serial.print(voltage, 2);
    Serial.println(" V");
  }
}
