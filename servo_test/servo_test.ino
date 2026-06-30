#include <ESP32Servo.h>

const int SERVO_PIN = 13;      // KC868-A6 SERIAL connector: IO13 / GPIO13

// Number of full cycles:
// 0° -> 180° -> 0° = one cycle
int n = 3;

const int MIN_ANGLE = 0;
const int MAX_ANGLE = 180;

const int STEP_DEGREES = 1;
const int STEP_DELAY_MS = 12;

Servo mg90s;

void moveSmoothly(int startAngle, int endAngle) {
  int stepDirection = (endAngle >= startAngle) ? STEP_DEGREES : -STEP_DEGREES;

  for (int angle = startAngle;
       (stepDirection > 0) ? angle <= endAngle : angle >= endAngle;
       angle += stepDirection) {

    mg90s.write(angle);
    delay(STEP_DELAY_MS);
  }
}

void moveServoCycles(int cycles) {
  if (cycles <= 0) {
    return;
  }

  for (int cycle = 1; cycle <= cycles; cycle++) {
    Serial.printf("Movement cycle %d of %d\n", cycle, cycles);

    // Move from 0° to 180°
    moveSmoothly(MIN_ANGLE, MAX_ANGLE);
    delay(300);

    // Return from 180° to 0°
    moveSmoothly(MAX_ANGLE, MIN_ANGLE);
    delay(500);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // Typical safe pulse-width range for a hobby positional servo.
  // Adjust only if your particular MG90S does not reach the desired ends.
  mg90s.setPeriodHertz(50);
  mg90s.attach(SERVO_PIN, 500, 2500);

  // Start in a known position.
  mg90s.write(MIN_ANGLE);
  delay(700);

  Serial.println("KC868-A6 MG90S test started.");
}

void loop() {
  moveServoCycles(n);

  Serial.println("Sequence completed.");
  delay(5000);

  // Prevent continuous repeats.
  // Remove this line if you want it to repeat forever.
  while (true) {
    delay(1000);
  }
}