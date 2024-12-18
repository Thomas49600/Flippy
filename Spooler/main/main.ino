#include <Servo.h>

Servo myservo;

#define TOTAL_WINDINGS 1885 //total number of loops of copper wire
#define MAX_SERVO 86 // adjust to set the max servo angle
#define MIN_SERVO 10 // adjust to set the min servo angle
#define BASE_SERVO_FACTOR 4 // the basic speed of the servo moving

int stepCount = 0; // Counter to keep track of steps
int servoPos = MIN_SERVO; // Starting position of the servo
bool movingDown = true; // Flag to track direction of servo movement

/*
>> Make sure servo and motor are on separate timers <<
Timer0: Pins 5 and 6
Timer1: Pins 9 and 10
Timer2: Pins 3 and 11

The above is for an Arduino UNO
*/

// Define the pin numbers
const int enPin = 11;   // Enable pin for motor
const int in1Pin = 4;   // Input pin 1 for motor
const int in2Pin = 3;   // Input pin 2 for motor
const int irSensorPin = 2; // IR sensor pin

volatile unsigned long pulseCount = 0; // Counter for IR sensor pulses
unsigned long lastPulseTime = 0; // Time of the last detected pulse
unsigned long debounceDelay = 1.5; // Debounce delay in milliseconds

void setup() {
  // Set pin modes
  pinMode(enPin, OUTPUT);
  pinMode(in1Pin, OUTPUT);
  pinMode(in2Pin, OUTPUT);
  pinMode(irSensorPin, INPUT);
  
  // Initialize motor direction
  digitalWrite(in1Pin, HIGH);
  digitalWrite(in2Pin, LOW);
  
  // Attach interrupt to the IR sensor pin
  attachInterrupt(digitalPinToInterrupt(irSensorPin), countPulse, RISING);

  myservo.attach(9); // Attach the servo to pin 9
  myservo.write(MIN_SERVO);
  
  Serial.begin(9600); // Initialize serial communication for debugging
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void loop() {
  pulseCount = 0; // Reset pulse count

  int delayTime = 6000;
  int steps = 100; // Number of steps to change the speed
  int stepDelay = delayTime / steps; // Time for each step

  // Accelerate to full speed over X seconds
  for (int i = 0; i <= steps; i++) {
    Serial.println(pulseCount);
    int speed = map(i, 0, steps, 40, 255); // Map step to PWM value (min-255)
    analogWrite(enPin, speed); // Set motor speed
    delay(stepDelay); // Wait for the next step

    // Move the servo during acceleration
    if (stepCount % 2 == 0) {
      if (movingDown) {
        servoPos--; // Move the servo down
        if (servoPos <= MIN_SERVO) {
          movingDown = false; // Change direction when reaching MIN_SERVO
        }
      } else {
        servoPos++; // Move the servo up
        if (servoPos >= MAX_SERVO) {
          movingDown = true; // Change direction when reaching MAX_SERVO
        }
      }
      myservo.write(servoPos); // Move the servo to the new position
    }
    stepCount++;
  }

  // Rotate the motor X number of times
  unsigned long targetPulses = TOTAL_WINDINGS - 200;
  while (pulseCount < targetPulses) {
    Serial.println(pulseCount);
    /*
    Serial.print(2);
    Serial.print(", ");
    Serial.print(10);
    Serial.print(", ");
    Serial.println(mapFloat(servoPos, MIN_SERVO, MAX_SERVO, 0, 3.14159265359));
    */
    delay(2); // speed control of motor

    int mod_factor = BASE_SERVO_FACTOR*sin(mapFloat(servoPos, MIN_SERVO, MAX_SERVO, 0, 3.14159265359));
    if (mod_factor < 2) {
      mod_factor = 2;
    }
    // Move the servo while rotating
    if (stepCount % mod_factor == 0) { // 0.01745329251 = pi/180, 0.03490658503 = 2pi/180 = pi/90
      if (movingDown) {
        servoPos--; // Move the servo down
        if (servoPos <= MIN_SERVO) {
          movingDown = false; // Change direction when reaching MIN_SERVO
        }
      } else {
        servoPos++; // Move the servo up
        if (servoPos >= MAX_SERVO) {
          movingDown = true; // Change direction when reaching MAX_SERVO
        }
      }
      myservo.write(servoPos); // Move the servo to the new position
    }
    stepCount++;
  }

  // Decelerate to stop over X seconds
  for (int i = steps; i >= 0; i--) {
    Serial.println(pulseCount);
    int speed = map(i, 0, steps, 0, 255); // Map step to PWM value (0-255)
    analogWrite(enPin, speed); // Set motor speed
    delay(stepDelay); // Wait for the next step

    // Move the servo during deceleration
    if (stepCount % 2 == 0) {
      if (movingDown) {
        servoPos--; // Move the servo down
        if (servoPos <= MIN_SERVO) {
          movingDown = false; // Change direction when reaching MIN_SERVO
        }
      } else {
        servoPos++; // Move the servo up
        if (servoPos >= MAX_SERVO) {
          movingDown = true; // Change direction when reaching MAX_SERVO
        }
      }
      myservo.write(servoPos); // Move the servo to the new position
    }
    stepCount++;
  }

  Serial.print("Total rotations: ");
  Serial.println(pulseCount);
  //servoPos = MIN_SERVO;
  //myservo.write(servoPos);
  delay(20000000000000); // Wait
}

// Interrupt service routine for counting IR sensor pulses
void countPulse() {
  unsigned long currentTime = millis();
  if (currentTime - lastPulseTime > debounceDelay) {
    pulseCount++;
    lastPulseTime = currentTime;
  }
}

