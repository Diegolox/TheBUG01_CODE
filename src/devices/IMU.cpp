#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BNO055.h>
#include "devices/IMU.h"
#include "pines.h"

static constexpr uint8_t bnoAddress = 0x29;
static Adafruit_BNO055 bno(55, bnoAddress, &Wire);

bool initIMU(bool modoInercial) {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    return bno.begin(modoInercial ? OPERATION_MODE_IMUPLUS : OPERATION_MODE_NDOF);
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

GiroBNO055 leerGiroIMU() {
    const imu::Vector<3> giro = bno.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);
    return {static_cast<float>(giro.x()), static_cast<float>(giro.y()),
            static_cast<float>(giro.z())};
}

bool leerBalanceIMU(float &rollDeg, float &giroYDegPorSegundo) {
    // Lectura consecutiva desde GYR_DATA_Y_LSB (0x16) hasta EUL_ROLL_MSB (0x1d).
    constexpr uint8_t gyroYRegister = Adafruit_BNO055::BNO055_GYRO_DATA_Y_LSB_ADDR;
    constexpr size_t length = 8;
    Wire.beginTransmission(bnoAddress);
    Wire.write(gyroYRegister);
    if (Wire.endTransmission(false) != 0 || Wire.requestFrom(bnoAddress, length) != length) {
        return false;
    }

    uint8_t data[length];
    for (uint8_t &part : data) {
        const int value = Wire.read();
        if (value < 0) {
            return false;
        }
        part = static_cast<uint8_t>(value);
    }

    const int16_t gyroRaw = static_cast<int16_t>(data[0] | (data[1] << 8));
    const int16_t rollRaw = static_cast<int16_t>(data[6] | (data[7] << 8));
    giroYDegPorSegundo = gyroRaw / 16.0f;
    rollDeg = rollRaw / 16.0f;
    return true;
}
