#include <Arduino.h>
#include <BluetoothSerial.h>
#include <math.h>
#include "devices/IMU.h"
#include "hal/board.h"

namespace {

constexpr char bluetoothName[] = "TheBUG01-Inclinacion";
constexpr float initialRollReference = -4.44f;
constexpr float captureMaxGyroDegPerSec = 2.0f;
constexpr float trimStepDeg = 0.1f;
constexpr int maxTrimSteps = 20;
constexpr uint32_t samplePeriodUs = 10000;
constexpr uint32_t reportPeriodUs = 50000;

BluetoothSerial bluetooth;
bool imuReady = false;
bool bluetoothReady = false;
float baseRollReference = initialRollReference;
int trimSteps = 0;
bool captureRequested = false;
uint32_t lastSampleAt = 0;
uint32_t lastReportAt = 0;

void readCommand(Stream &port) {
    if (port.available() <= 0) {
        return;
    }

    switch (port.read()) {
    case '+':
        trimSteps = min(trimSteps + 1, maxTrimSteps);
        break;
    case '-':
        trimSteps = max(trimSteps - 1, -maxTrimSteps);
        break;
    case 'c':
        captureRequested = true;
        break;
    case '0':
        captureRequested = false;
        baseRollReference = initialRollReference;
        trimSteps = 0;
        break;
    case 'x':
        port.println("Motores desarmados");
        break;
    }
}

} // namespace

void setup() {
    initBoard();
    Serial.begin(115200);
    imuReady = initIMU(true);
    if (!imuReady) {
        Serial.println("BNO055: no se pudo iniciar en I2C 0x29");
        return;
    }

    bluetoothReady = bluetooth.begin(bluetoothName);
    if (bluetoothReady) {
        Serial.printf("Bluetooth: %s\n", bluetoothName);
    } else {
        Serial.println("Bluetooth no disponible. Usa el monitor USB.");
    }
    Serial.println("Motores desarmados | c: capturar Roll | + / -: ajuste 0.1 grados | 0: referencia inicial");
    lastSampleAt = micros();
    lastReportAt = lastSampleAt;
}

void loop() {
    if (!imuReady) {
        delay(100);
        return;
    }

    readCommand(Serial);
    const bool clientConnected = bluetoothReady && bluetooth.hasClient();
    if (clientConnected) {
        readCommand(bluetooth);
    }

    const uint32_t now = micros();
    const uint32_t elapsed = now - lastSampleAt;
    if (elapsed < samplePeriodUs) {
        return;
    }
    lastSampleAt = now;

    const DatosBNO055 orientation = leerIMU();
    const GiroBNO055 gyro = leerGiroIMU();
    if (captureRequested) {
        captureRequested = false;
        const bool stationary = fabsf(gyro.x) <= captureMaxGyroDegPerSec &&
                                fabsf(gyro.y) <= captureMaxGyroDegPerSec &&
                                fabsf(gyro.z) <= captureMaxGyroDegPerSec;
        if (stationary) {
            baseRollReference = orientation.roll;
            trimSteps = 0;
        }
        const char *result = stationary ? "Referencia capturada" : "Captura rechazada: manten quieto el chasis";
        Serial.println(result);
        if (clientConnected) {
            bluetooth.println(result);
        }
    }
    if (now - lastReportAt < reportPeriodUs) {
        return;
    }
    lastReportAt = now;

    const float rollReference = baseRollReference + trimSteps * trimStepDeg;
    const float lean = rollReference - orientation.roll;
    char line[144];
    snprintf(line, sizeof(line),
             "Roll=%7.2f Ref=%7.2f Incl=%+7.2f | Giro X=%7.2f Y=%7.2f Z=%7.2f | dt=%.2f ms\r\n",
             orientation.roll, rollReference, lean, gyro.x, gyro.y, gyro.z, elapsed / 1000.0);
    Serial.print(line);
    if (clientConnected) {
        bluetooth.print(line);
    }
}
