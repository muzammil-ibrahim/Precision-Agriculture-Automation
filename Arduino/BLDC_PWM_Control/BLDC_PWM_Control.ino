//#######################################
// GPIO mappings for Arduino UNO
//#######################################
int m1_EL_Start_Stop = 4;  // EL (digital) → OK
int m1_Signal_hall   = 2;  // Hall sensor input → OK, D2 supports external interrupt (INT0)
int m1_ZF_Direction  = 7;  // ZF (digital) → OK
int m1_VR_speed      = 3;  // VR (PWM) → OK, D3 supports analogWrite()


// Pixhawk input pins
int pixhawkSpeedPin     = 9;  // PWM input from Pixhawk (any digital OK)
int pixhawkDirectionPin = 11;  // PWM input from Pixhawk
int pixhawkStepsPin     = 10; // PWM input from Pixhawk

//#######################################
volatile int pos = 0;   // must be volatile (ISR modifies it)
int steps = 0;
int speed1 = 0;
String direction1;
//#######################################

// ISR for hall sensor
void plus() {
  pos++; // count steps
  if (pos >= steps && steps > 0) {
    wheelStop();
    pos = 0;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(m1_EL_Start_Stop, OUTPUT);
  pinMode(m1_Signal_hall, INPUT);
  pinMode(m1_ZF_Direction, OUTPUT);

  pinMode(pixhawkSpeedPin, INPUT);
  pinMode(pixhawkDirectionPin, INPUT);
  pinMode(pixhawkStepsPin, INPUT);

  digitalWrite(m1_EL_Start_Stop, LOW);
  digitalWrite(m1_ZF_Direction, LOW);
  analogWrite(m1_VR_speed, 0);

  attachInterrupt(digitalPinToInterrupt(m1_Signal_hall), plus, CHANGE);
}

void loop() {
    delay(300);
  // ---- Read Pixhawk PWM ----
  unsigned long speedPulse = pulseIn(pixhawkSpeedPin, HIGH, 25000);
  unsigned long dirPulse   = pulseIn(pixhawkDirectionPin, HIGH, 25000);
  unsigned long stepPulse  = pulseIn(pixhawkStepsPin, HIGH, 25000);




  // Map speed (1000–2000 µs → 0–255 PWM)
  if (speedPulse > 900 && speedPulse < 2100) {
    Serial.print("Raw speed PWM: "); Serial.println(speedPulse);
    speed1 = map(speedPulse, 1000, 2000, 0, 255);
    speed1 = constrain(speed1, 0, 255);
  }

  // Map direction
  if (dirPulse > 900 && dirPulse < 2100) {
    Serial.print("Raw direction PWM: "); Serial.println(dirPulse);
    if (dirPulse < 1300) direction1 = "backward";
    else if (dirPulse > 1700) direction1 = "forward";
    else direction1 = "stop";
  }

  // Map steps (1000–2000 µs → 0–1000 steps)
  if (stepPulse > 900 && stepPulse < 2100) {
    steps = map(stepPulse, 1000, 2000, 0, 1000);
    steps = constrain(steps, 0, 1000);
  }

  // ---- Debugging ----
  // Serial.print("Speed1: "); Serial.print(speed1);
  // Serial.print("  Dir: "); Serial.print(direction1);
  // Serial.print("  Steps: "); Serial.print(steps);
  // Serial.print("  Pos: "); Serial.println(pos);

  // ---- Drive logic ----
  drive();
}

//#######################################
// Motor control
//#######################################
void drive() {
  if (steps == 0) {
    // Ignore step count, run continuously
    if(direction1 == "forward") wheelMoveForward();
    else if(direction1 == "backward") wheelMoveBackward();
    else wheelStop();
  } else {
    // Step-limited mode
    if(direction1 == "forward" && pos < steps) wheelMoveForward();
    else if(direction1 == "backward" && pos < steps) wheelMoveBackward();
    else wheelStop();
  }
}


void wheelStop() {
  digitalWrite(m1_EL_Start_Stop, LOW);
}

void wheelMoveForward() {
  analogWrite(m1_VR_speed, speed1);
  //digitalWrite(m1_EL_Start_Stop, LOW);
  digitalWrite(m1_ZF_Direction, HIGH);
  digitalWrite(m1_EL_Start_Stop, HIGH);
}

void wheelMoveBackward() {
  analogWrite(m1_VR_speed, speed1);
  //digitalWrite(m1_EL_Start_Stop, LOW);
  digitalWrite(m1_ZF_Direction, LOW);
  digitalWrite(m1_EL_Start_Stop, HIGH);
}
