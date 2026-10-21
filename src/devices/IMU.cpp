#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BNO055.h>
#include "config.h"
#include "devices/IMU.h"
#include "pines.h"

static Adafruit_BNO055 bno(55, 0x29, &Wire);

bool initIMU() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    return bno.begin();
}

DatosBNO055 leerIMU() {
    DatosBNO055 datos;

    imu::Vector<3> orientacion =
        bno.getVector(Adafruit_BNO055::VECTOR_EULER);

    imu::Vector<3> aceleracion =
        bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);

    datos.rumbo = orientacion.x();
    datos.roll  = orientacion.y();
    datos.pitch = orientacion.z();

    datos.accX = aceleracion.x();
    datos.accY = aceleracion.y();
    datos.accZ = aceleracion.z();

    return datos;
}