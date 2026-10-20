#include <devices/IMU.h>
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BNO055.h>

Adafruit_BNO055 bno(55, 0x28);


void initIMU(){
    if (!bno.begin()) {
        Serial.println("No se detecta el BNO055");
        while (true) delay(100);
    }

    Serial.println("BNO055 conectado");
}