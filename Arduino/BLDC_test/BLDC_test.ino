//#######################################
// Pin mappings for Arduino UNO
//#######################################
int m1_EL_Start_Stop = 4;  // EL (digital)
int m1_ZF_Direction  = 7;  // ZF (digital)
int m1_VR_speed      = 3;  // VR (PWM output)

int speed1 = 130;  // PWM speed (0–255)

void setup() {
  pinMode(m1_EL_Start_Stop, OUTPUT);
  pinMode(m1_ZF_Direction, OUTPUT);
  pinMode(m1_VR_speed, OUTPUT);

  // ✅ Motor OFF at startup
  wheelStop();

  delay(1000); // wait 1s before starting

  // ✅ Run motor forward for 2 seconds
  wheelMoveForward();
  delay(2000);

  // ✅ Stop motor
  wheelStop();
}

void loop() {
  // nothing here
}

void wheelStop() {
  analogWrite(m1_VR_speed, 0);
  digitalWrite(m1_EL_Start_Stop, LOW);
}

void wheelMoveForward() {
  analogWrite(m1_VR_speed, speed1);
  digitalWrite(m1_EL_Start_Stop, LOW);
  digitalWrite(m1_ZF_Direction, HIGH);
  digitalWrite(m1_EL_Start_Stop, HIGH);
}
