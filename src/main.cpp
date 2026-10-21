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
    DatosBNO055 datos = leerIMU();

    Serial.printf("Rumbo: %.2f | Roll: %.2f | Pitch: %.2f\n",
                  datos.rumbo, datos.roll, datos.pitch);

    Serial.printf("AccX: %.2f | AccY: %.2f | AccZ: %.2f\n",
                  datos.accX, datos.accY, datos.accZ);

    delay(100);
  
  }