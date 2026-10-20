#include <devices/IMU.h>
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BNO055.h>

Adafruit_BNO055 bno(55, 0x28);

void initIMU() {
    Wire.begin(21, 22);  // SDA, SCL. Cambia los pines si usas otros.

    while (!bno.begin()) {
        Serial.println("BNO055 no detectado. Reintentando...");
        delay(1000);
    }

    Serial.println("BNO055 conectado");
}