#include <Arduino.h>
#include <hal/board.h>
#include <devices/buzzer.h>
#include <devices/IMU.h>
#include <Wire.h>


void setup() {
  initBoard();
  Serial.begin(115200);
  initIMU();
}

void loop() {
  delay(2000);
}