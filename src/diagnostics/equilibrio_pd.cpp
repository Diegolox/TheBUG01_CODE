#include <Arduino.h>
#include <BluetoothSerial.h>
#include <Wire.h>
#include <atomic>
#include <math.h>
#include "devices/IMU.h"
#include "devices/encoders.h"
#include "devices/motores.h"
#include "hal/board.h"

namespace {

constexpr uint32_t periodMs = 10;
constexpr uint32_t maxPeriodUs = 20000;
constexpr uint32_t trialUs = 8000000;
constexpr uint32_t reportMs = 50;
constexpr float initialReferenceDeg = -4.44f;
constexpr float trimStepDeg = 0.1f;
constexpr int maxTrimSteps = 20;
constexpr float initialKp = 5.0f;   // cuentas PWM por grado
constexpr float initialKd = 0.55f;  // cuentas PWM por grado/s
constexpr float pwmCompensation = 8.0f;
constexpr float compensationRamp = 12.0f;
constexpr int maxPwm = 64;  // 25 %
constexpr float armRateDegPerSec = 10.0f;
constexpr float tripAngleDeg = 15.0f;
constexpr float mechanicalLimitDeg = 18.0f;

struct Command {
    int key;
    bool viaBluetooth;
};

struct Telemetry {
    float roll;
    float reference;
    float lean;
    float rate;
    uint32_t periodUs;
    int pwm;
    const char *state;
    bool armRejected;
    float rejectedLean;
    float rejectedRate;
    float kp;
    float kd;
    bool saturated;
    uint32_t sampleAtUs;
    EncoderReadings encoders;
};

struct ReportState {
    uint32_t lastReportMs = 0;
    uint32_t sampleAtUs = 0;
    EncoderReadings encoders{};
    bool hasSample = false;
};

BluetoothSerial bluetooth;
bool bluetoothReady = false;
QueueHandle_t commands = nullptr;
QueueHandle_t usbTelemetry = nullptr;
QueueHandle_t btTelemetry = nullptr;
std::atomic<uint32_t> stopRequested{0};

// Estado de control, usado solo desde su tarea.
float baseReference = initialReferenceDeg;
float kp = initialKp;
float kd = initialKd;
int trimSteps = 0;
bool armed = false;
bool armedByBluetooth = false;
uint32_t armedAt = 0;
const char *state = "Desarmado";
bool armRejected = false;
float rejectedLean = 0.0f;
float rejectedRate = 0.0f;

float reference() {
    return baseReference + trimSteps * trimStepDeg;
}

void disarm(const char *reason) {
    armed = false;
    armRejected = false;
    state = reason;
}

void applyCommand(const Command &command, float roll, float rate, uint32_t now) {
    switch (command.key) {
    case '+':
        trimSteps = min(trimSteps + 1, maxTrimSteps);
        break;
    case '-':
        trimSteps = max(trimSteps - 1, -maxTrimSteps);
        break;
    case '0':
        baseReference = initialReferenceDeg;
        trimSteps = 0;
        break;
    case 'p':
    case 'P':
        kp = constrain(kp + (command.key == 'p' ? 0.5f : -0.5f), 2.0f, 8.0f);
        break;
    case 'd':
    case 'D':
        kd = constrain(kd + (command.key == 'd' ? 0.05f : -0.05f), 0.10f, 1.0f);
        break;
    case 'c':
        if (!armed && fabsf(rate) <= 2.0f && fabsf(roll - initialReferenceDeg) <= 8.0f) {
            baseReference = roll;
            trimSteps = 0;
            armRejected = false;
            state = "Referencia capturada";
        }
        break;
    case 'a': {
        if (armed) {
            break;
        }
        const float lean = reference() - roll;
        const char *reason = nullptr;
        if (fabsf(rate) > armRateDegPerSec) {
            reason = "Armado rechazado: GiroY";
        } else if (command.viaBluetooth && !bluetooth.hasClient()) {
            reason = "Armado rechazado: Bluetooth desconectado";
        }

        armRejected = reason != nullptr;
        if (armRejected) {
            rejectedLean = lean;
            rejectedRate = rate;
            state = reason;
            break;
        }

        armed = true;
        armedByBluetooth = command.viaBluetooth;
        armedAt = now;
        state = "Armado";
        break;
    }
    }
}

void controlTask(void *) {
    TickType_t nextTick = xTaskGetTickCount();
    uint32_t lastSample = micros();

    for (;;) {
        vTaskDelayUntil(&nextTick, pdMS_TO_TICKS(periodMs));
        const uint32_t now = micros();
        const uint32_t elapsed = now - lastSample;
        lastSample = now;

        float roll = NAN;
        float rate = NAN;
        const bool valid = leerBalanceIMU(roll, rate);
        const bool stop = stopRequested.exchange(0) != 0;
        if (!valid || elapsed > maxPeriodUs || stop) {
            disarm(!valid ? "Corte: lectura IMU" :
                   elapsed > maxPeriodUs ? "Corte: periodo de control" : "Parada manual");
            xQueueReset(commands);
        } else {
            Command command;
            for (int i = 0; i < 12 && xQueueReceive(commands, &command, 0) == pdTRUE; ++i) {
                applyCommand(command, roll, rate, now);
            }
        }

        const float target = reference();
        const float lean = target - roll;
        if (armed) {
            if (armedByBluetooth && !bluetooth.hasClient()) {
                disarm("Corte: Bluetooth desconectado");
            } else if (now - armedAt >= trialUs) {
                disarm("Corte: fin de ensayo");
            } else if (fabsf(lean) > tripAngleDeg || fabsf(roll - initialReferenceDeg) > mechanicalLimitDeg) {
                disarm("Corte: inclinacion");
            }
        }

        int pwm = 0;
        bool saturated = false;
        if (armed) {
            // Un mando positivo del PD pide avance, que en ambos drivers es PWM negativo.
            const float request = -(kp * lean + kd * rate);
            const float magnitude = fabsf(request);
            // La compensacion crece de forma continua desde PWM cero.
            const float compensated = magnitude +
                pwmCompensation * min(magnitude / compensationRamp, 1.0f);
            saturated = compensated > maxPwm;
            pwm = static_cast<int>(lroundf(copysignf(min(compensated, static_cast<float>(maxPwm)),
                                                    request)));
        }
        setMotorPwm(Motor::M1, pwm);
        setMotorPwm(Motor::M2, pwm);

        const Telemetry reading{roll, target, lean, rate, elapsed, pwm, state,
                                armRejected, rejectedLean, rejectedRate, kp, kd, saturated,
                                now, readEncoders()};
        xQueueOverwrite(usbTelemetry, &reading);
        xQueueOverwrite(btTelemetry, &reading);
    }
}

void servicePort(Stream &port, QueueHandle_t telemetry, bool viaBluetooth, ReportState &report) {
    for (int i = 0; i < 8 && port.available() > 0; ++i) {
        const int key = port.read();
        if (key == 'x') {
            stopRequested.store(1);
        } else if (key == 'a' || key == '+' || key == '-' || key == 'c' || key == '0' ||
                   key == 'p' || key == 'P' || key == 'd' || key == 'D') {
            const Command command{key, viaBluetooth};
            xQueueSend(commands, &command, 0);
        }
    }

    const uint32_t now = millis();
    Telemetry reading;
    if (now - report.lastReportMs >= reportMs && xQueueReceive(telemetry, &reading, 0) == pdTRUE) {
        report.lastReportMs = now;
        const uint32_t sampleDeltaUs = reading.sampleAtUs - report.sampleAtUs;
        float rightSpeed = NAN;
        float leftSpeed = NAN;
        if (report.hasSample && sampleDeltaUs > 0) {
            const float countsToPerSecond = 1000000.0f / sampleDeltaUs;
            const int64_t rightDelta = static_cast<int64_t>(reading.encoders.m1.count) -
                                       report.encoders.m1.count;
            const int64_t leftDelta = static_cast<int64_t>(reading.encoders.m2.count) -
                                      report.encoders.m2.count;
            // M1 cuenta en positivo al avanzar; M2 tiene el signo inverso.
            rightSpeed = rightDelta * countsToPerSecond;
            leftSpeed = -leftDelta * countsToPerSecond;
        }
        report.sampleAtUs = reading.sampleAtUs;
        report.encoders = reading.encoders;
        report.hasSample = true;
        char rejection[76] = "";
        if (reading.armRejected) {
            snprintf(rejection, sizeof(rejection), " | Al pedir armado: Incl=%+.2f GiroY=%+.2f",
                     reading.rejectedLean, reading.rejectedRate);
        }
        port.printf("Roll=%6.2f Ref=%6.2f Incl=%+6.2f GiroY=%+7.2f | PWM=%+d Sat=%d | Vder=%+.0f Vizq=%+.0f | Kp=%.2f Kd=%.2f | dt=%.2f ms | %s%s\r\n",
                    reading.roll, reading.reference, reading.lean, reading.rate,
                    reading.pwm, reading.saturated, rightSpeed, leftSpeed, reading.kp, reading.kd,
                    reading.periodUs / 1000.0, reading.state, rejection);
    }
}

void bluetoothTask(void *) {
    ReportState report;
    for (;;) {
        if (bluetooth.hasClient()) {
            servicePort(bluetooth, btTelemetry, true, report);
        } else {
            report.hasSample = false;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

} // namespace

void setup() {
    initBoard();
    initEncoders();
    Serial.begin(115200);
    if (!initIMU(true)) {
        Serial.println("BNO055 no disponible. Motores desarmados.");
        return;
    }
    Wire.setTimeOut(5);

    commands = xQueueCreate(12, sizeof(Command));
    usbTelemetry = xQueueCreate(1, sizeof(Telemetry));
    btTelemetry = xQueueCreate(1, sizeof(Telemetry));
    if (!commands || !usbTelemetry || !btTelemetry) {
        Serial.println("No se pudieron crear las colas. Motores desarmados.");
        return;
    }

    bluetoothReady = bluetooth.begin("TheBUG01-PD");
    const BaseType_t core = xPortGetCoreID();
    if (xTaskCreatePinnedToCore(controlTask, "equilibrio", 4096, nullptr, 3, nullptr, core) != pdPASS) {
        Serial.println("No se pudo iniciar el control. Motores desarmados.");
        return;
    }
    if (bluetoothReady &&
        xTaskCreatePinnedToCore(bluetoothTask, "pd-bluetooth", 4096, nullptr, 1, nullptr, core) != pdPASS) {
        bluetoothReady = false;
        bluetooth.end();
    }

    Serial.println("PD | a: armar 8 s | x: cortar | c: capturar desarmado | + / -: ajustar | 0: restaurar");
    Serial.println("p/P: subir/bajar Kp | d/D: subir/bajar Kd");
    Serial.println("Vder y Vizq en cuentas de encoder/s, positivas al avanzar");
    Serial.println(bluetoothReady ? "Bluetooth: TheBUG01-PD" : "Solo USB");
}

void loop() {
    static ReportState report;
    if (commands && usbTelemetry && btTelemetry) {
        servicePort(Serial, usbTelemetry, false, report);
    }
    delay(1);
}
