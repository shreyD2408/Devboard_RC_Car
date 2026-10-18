
#include <Bluepad32.h>
#include <ESP32Servo.h>

#define LEFT_EN 25
#define LEFT_IN1 26
#define LEFT_IN2 27

#define RIGHT_EN 14
#define RIGHT_IN1 16
#define RIGHT_IN2 17

#define SERVO_PIN 18
#define HEADLIGHT_PIN 21
#define BRAKELIGHT_PIN 22

#define DEADZONE 40
#define MAX_SPEED 200
#define MAX_STEERING 40
#define BRAKE_THRESHOLD 100

ControllerPtr controller = nullptr;
Servo steeringServo;

bool headlightsOn = false;
bool previousHeadlightButton = false;

int previousThrottle = 0;

void stopCar() {
  analogWrite(LEFT_EN, 0);
  analogWrite(RIGHT_EN, 0);

  digitalWrite(LEFT_IN1, LOW);
  digitalWrite(LEFT_IN2, LOW);
  digitalWrite(RIGHT_IN1, LOW);
  digitalWrite(RIGHT_IN2, LOW);

  digitalWrite(BRAKELIGHT_PIN, HIGH);
}

void setMotor(int enablePin, int in1, int in2, int speed) {
  speed = constrain(speed, -255, 255);

  if (speed > 0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  } else if (speed < 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  } else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }

  analogWrite(enablePin, abs(speed));
}

void onConnectedController(ControllerPtr ctl) {
  if (ctl->isGamepad() && controller == nullptr) {
    controller = ctl;
    Serial.println("PS5 controller connected!");
  }
}

void onDisconnectedController(ControllerPtr ctl) {
  if (controller == ctl) {
    controller = nullptr;
    stopCar();
    steeringServo.write(90);
    previousThrottle = 0;
    Serial.println("Controller disconnected!");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(LEFT_EN, OUTPUT);
  pinMode(LEFT_IN1, OUTPUT);
  pinMode(LEFT_IN2, OUTPUT);

  pinMode(RIGHT_EN, OUTPUT);
  pinMode(RIGHT_IN1, OUTPUT);
  pinMode(RIGHT_IN2, OUTPUT);

  pinMode(HEADLIGHT_PIN, OUTPUT);
  pinMode(BRAKELIGHT_PIN, OUTPUT);

  digitalWrite(HEADLIGHT_PIN, LOW);
  digitalWrite(BRAKELIGHT_PIN, HIGH);

  steeringServo.setPeriodHertz(50);
  steeringServo.attach(SERVO_PIN, 500, 2400);
  steeringServo.write(90);

  stopCar();

  BP32.setup(
    &onConnectedController,
    &onDisconnectedController
  );

  Serial.println("RC car ready.");
  Serial.println("Put your PS5 controller into pairing mode.");
}

void loop() {
  BP32.update();

  if (controller == nullptr || !controller->isConnected()) {
    stopCar();
    steeringServo.write(90);
    delay(10);
    return;
  }

  // LEFT JOYSTICK: FORWARD AND REVERSE
  int throttle = -controller->axisY();

  if (abs(throttle) < DEADZONE) {
    throttle = 0;
  }

  int motorSpeed = map(
    abs(throttle), 0, 512, 0, MAX_SPEED
  );

  if (throttle < 0) {
    motorSpeed = -motorSpeed;
  }

  // RIGHT JOYSTICK: STEERING
  int steeringInput = controller->axisRX();

  if (abs(steeringInput) < DEADZONE) {
    steeringInput = 0;
  }

  int steeringAngle = map(
    steeringInput,
    -512, 512,
    90 - MAX_STEERING,
    90 + MAX_STEERING
  );

  steeringServo.write(
    constrain(steeringAngle, 0, 180)
  );

  // TRIANGLE: TOGGLE HEADLIGHTS
  bool headlightButton = controller->y();

  if (headlightButton && !previousHeadlightButton) {
    headlightsOn = !headlightsOn;

    digitalWrite(
      HEADLIGHT_PIN,
      headlightsOn ? HIGH : LOW
    );
  }

  previousHeadlightButton = headlightButton;

  // L2: BRAKE
  bool braking = controller->brake() > BRAKE_THRESHOLD;

  if (braking) {
    motorSpeed = 0;
  }

  // DRIVE MOTORS
  setMotor(
    LEFT_EN, LEFT_IN1, LEFT_IN2, motorSpeed
  );

  setMotor(
    RIGHT_EN, RIGHT_IN1, RIGHT_IN2, motorSpeed
  );

  // BRAKE LIGHTS
  bool slowingDown =
    (previousThrottle != 0 && throttle == 0);

  digitalWrite(
    BRAKELIGHT_PIN,
    (braking || slowingDown) ? HIGH : LOW
  );

  previousThrottle = throttle;

  delay(10);
}
