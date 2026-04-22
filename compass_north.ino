// ============================================
// Motorized Compass - Always Points North
// ESP32-C3 + GY-271 (QMC5883L) + A4988
// ============================================

#include <Wire.h>
#include <QMC5883LCompass.h>

// --- A4988 Pins ---
#define STEP_PIN  4
#define DIR_PIN   5

// --- I2C Pins (ESP32-C3) ---
#define SDA_PIN   6
#define SCL_PIN   7

// --- Stepper Settings ---
#define STEPS_PER_REV   200   // 1.8 degree stepper = 200 steps/rev
                               // change to 2048 if using 28BYJ-48
#define STEP_DELAY_US   1200  // microseconds between steps (speed)
#define DEADBAND        3     // degrees of tolerance before moving

QMC5883LCompass compass;

// Track current motor position in degrees
float currentAngle = 0.0;

// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(115200);

  // Stepper pins
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(STEP_PIN, LOW);
  digitalWrite(DIR_PIN, LOW);

  // I2C + Compass
  Wire.begin(SDA_PIN, SCL_PIN);
  compass.init();
  compass.setCalibration(-1500, 1500, -1500, 1500, -1500, 1500); // adjust after calibration

  Serial.println("Motorized Compass Ready");
}

// ============================================
// STEP MOTOR
// ============================================
void stepMotor(int steps, bool clockwise) {
  digitalWrite(DIR_PIN, clockwise ? HIGH : LOW);
  delayMicroseconds(10); // settle direction pin

  for (int i = 0; i < abs(steps); i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(STEP_DELAY_US);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(STEP_DELAY_US);
  }
}

// ============================================
// SHORTEST PATH ANGLE DIFFERENCE
// Returns -180 to +180
// ============================================
float angleDiff(float target, float current) {
  float diff = target - current;
  while (diff > 180)  diff -= 360;
  while (diff < -180) diff += 360;
  return diff;
}

// ============================================
// DEGREES TO STEPS
// ============================================
int degreesToSteps(float degrees) {
  return (int)((abs(degrees) / 360.0) * STEPS_PER_REV);
}

// ============================================
// MAIN LOOP
// ============================================
void loop() {
  // Read compass
  compass.read();
  float heading = compass.getAzimuth(); // 0-360 degrees, 0 = North

  Serial.print("Heading: ");
  Serial.print(heading);
  Serial.print("  Motor at: ");
  Serial.println(currentAngle);

  // To point North, motor needs to CANCEL OUT current heading
  // If facing 90deg (East), motor rotates -90deg to compensate
  float targetAngle = 360.0 - heading;
  if (targetAngle >= 360.0) targetAngle -= 360.0;

  // How far does the motor need to move?
  float error = angleDiff(targetAngle, currentAngle);

  // Only move if outside deadband
  if (abs(error) > DEADBAND) {
    int steps = degreesToSteps(error);
    bool clockwise = (error > 0);

    stepMotor(steps, clockwise);

    // Update tracked position
    currentAngle += (clockwise ? 1 : -1) * (steps * 360.0 / STEPS_PER_REV);
    while (currentAngle >= 360) currentAngle -= 360;
    while (currentAngle < 0)   currentAngle += 360;
  }

  delay(100); // read compass 10x per second
}
