#include <Arduino.h>
#include "devices/IMU.h"
#include "hal/board.h"

namespace {

constexpr uint32_t samplePeriodUs = 10000;
constexpr uint32_t reportPeriodUs = 50000;

bool imuReady = false;
uint32_t lastSampleAt = 0;
uint32_t lastReportAt = 0;

} // namespace

void setup() {
    initBoard();
    Serial.begin(115200);
    imuReady = initIMU(true);
    if (!imuReady) {
        Serial.println("BNO055: no se pudo iniciar en I2C 0x29");
        return;
    }

    Serial.println("BNO055 | modo IMU | angulos en grados | giro en grados/s");
    Serial.println("Sujeta el chasis y basculalo lentamente hacia delante y hacia atras");
    lastSampleAt = micros();
    lastReportAt = lastSampleAt;
}

void loop() {
    if (!imuReady) {
        delay(100);
        return;
    }

    const uint32_t now = micros();
    const uint32_t elapsed = now - lastSampleAt;
    if (elapsed < samplePeriodUs) {
        return;
    }
    lastSampleAt = now;

    const DatosBNO055 orientation = leerIMU();
    const GiroBNO055 gyro = leerGiroIMU();

    if (now - lastReportAt >= reportPeriodUs) {
        lastReportAt = now;
        Serial.printf("Rumbo=%7.2f Roll=%7.2f Pitch=%7.2f | Giro X=%7.2f Y=%7.2f Z=%7.2f | dt=%.2f ms\n",
                      orientation.rumbo, orientation.roll, orientation.pitch,
                      gyro.x, gyro.y, gyro.z, elapsed / 1000.0);
    }
}
