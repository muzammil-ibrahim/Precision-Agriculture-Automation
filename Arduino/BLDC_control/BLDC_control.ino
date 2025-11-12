#include <ArduinoJson.h>

//#######################################
// Pin mappings for Arduino UNO
//#######################################
int m1_EL_Start_Stop = 4;  // EL (digital)
int m1_Signal_hall   = 2;  // Hall sensor input (interrupt-capable)
int m1_ZF_Direction  = 7;  // ZF (digital)
int m1_VR_speed      = 3;  // VR (PWM output)

// {"direction":"forward","steps":30,"speed":50}
// {"direction":"backward","steps":30,"speed":50}
// {"direction":"stop","steps":0,"speed":0}

//#######################################
volatile int pos = 0;   // updated in ISR
int steps = 0;
int speed1 = 0;
String direction1 = "stop";  // default to stop
//#######################################

// ISR for hall sensor
void plus() {
  pos++; // count steps
  if (pos >= steps) {
    wheelStop();
    pos = 0;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(m1_EL_Start_Stop, OUTPUT);
  pinMode(m1_Signal_hall, INPUT);
  pinMode(m1_ZF_Direction, OUTPUT);
  pinMode(m1_VR_speed, OUTPUT);

  // ✅ Force motor OFF at startup
  digitalWrite(m1_EL_Start_Stop, LOW);
  digitalWrite(m1_ZF_Direction, LOW);
  analogWrite(m1_VR_speed, 0);

  attachInterrupt(digitalPinToInterrupt(m1_Signal_hall), plus, CHANGE);

  Serial.println("System ready. Waiting for command...");
}

void drive() {
  if (direction1 == "stop") {
    wheelStop();
    return;
  }

  if (direction1 == "forward" && pos < steps) {
    wheelMoveForward();
  } else if (direction1 == "backward" && pos < steps) {
    wheelMoveBackward();
  } else {
    wheelStop();
    pos = 0;
  }
}

void wheelStop() {
  analogWrite(m1_VR_speed, 0);         // zero speed
  digitalWrite(m1_EL_Start_Stop, LOW); // disable motor
}

void wheelMoveForward() {
  analogWrite(m1_VR_speed, speed1);
  digitalWrite(m1_EL_Start_Stop, LOW);
  digitalWrite(m1_ZF_Direction, HIGH);
  digitalWrite(m1_EL_Start_Stop, HIGH);
}

void wheelMoveBackward() {
  analogWrite(m1_VR_speed, speed1);
  digitalWrite(m1_EL_Start_Stop, LOW);
  digitalWrite(m1_ZF_Direction, LOW);
  digitalWrite(m1_EL_Start_Stop, HIGH);
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');  // read line

    StaticJsonDocument<200> doc; // fixed-size buffer
    DeserializationError error = deserializeJson(doc, command);

    if (!error) {
      direction1 = String((const char*)doc["direction"]);
      steps = doc["steps"] | 0;
      speed1 = doc["speed"] | 0;

      Serial.print("Dir: "); Serial.println(direction1);
      Serial.print("Steps: "); Serial.println(steps);
      Serial.print("Speed: "); Serial.println(speed1);

      drive();
    } else {
      Serial.println("JSON parse failed!");
    }
  }
}
