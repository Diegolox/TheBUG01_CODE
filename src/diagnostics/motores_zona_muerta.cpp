#include <Arduino.h>
#include "devices/encoders.h"
#include "devices/motores.h"
#include "hal/board.h"

namespace {

constexpr uint8_t maxPwmPercent = 60;
constexpr uint32_t restMs = 500;
constexpr uint32_t windowMs = 250;
constexpr uint32_t timeoutMs = 100000;
constexpr int32_t minCounts = 2;

enum class Phase { Idle, Rest, Ascending, Descending };

Phase phase = Phase::Idle;
Motor motor = Motor::M1;
int direction = 1;
uint8_t pwmPercent = 0;
uint32_t trialStartedAt = 0;
uint32_t windowStartedAt = 0;
int32_t windowStartCount = 0;
int32_t firstDelta = 0;
bool secondWindow = false;

int32_t motorCount() {
    const EncoderReadings readings = readEncoders();
    return motor == Motor::M1 ? readings.m1.count : readings.m2.count;
}

int dutyForPercent(uint8_t percent) {
    return (255 * percent + 50) / 100;
}

void beginWindow(uint32_t now, int32_t count) {
    windowStartedAt = now;
    windowStartCount = count;
}

void startStep(Phase next, uint8_t percent) {
    phase = next;
    pwmPercent = percent;
    secondWindow = false;
    const int duty = next == Phase::Rest ? 0 : direction * dutyForPercent(percent);
    setMotorPwm(motor, duty);
    beginWindow(millis(), motorCount());
}

void finish(const char *message) {
    setMotorPwm(motor, 0);
    phase = Phase::Idle;
    Serial.println(message);
}

void reportStep(int32_t secondDelta) {
    Serial.printf("%s | M%d | sentido=%+d | PWM=%u%% (%d/255) | cuentas=%ld,%ld\n",
                  phase == Phase::Ascending ? "Subida" : "Bajada",
                  static_cast<int>(motor), direction, static_cast<unsigned>(pwmPercent),
                  dutyForPercent(pwmPercent), static_cast<long>(firstDelta),
                  static_cast<long>(secondDelta));
}

void checkStep(int32_t secondDelta) {
    reportStep(secondDelta);
    if (phase == Phase::Ascending) {
        const bool sustainedMotion =
            (firstDelta >= minCounts && secondDelta >= minCounts) ||
            (firstDelta <= -minCounts && secondDelta <= -minCounts);
        if (sustainedMotion) {
            Serial.printf("ARRANQUE: %u%% (%d/255)\n", static_cast<unsigned>(pwmPercent),
                          dutyForPercent(pwmPercent));
            startStep(Phase::Descending, pwmPercent - 1);
        } else if (pwmPercent == maxPwmPercent) {
            finish("Sin arranque sostenido hasta el limite de PWM");
        } else {
            startStep(Phase::Rest, pwmPercent);
        }
        return;
    }

    if (firstDelta == 0 && secondDelta == 0) {
        Serial.printf("PARADA: %u%% (%d/255)\n", static_cast<unsigned>(pwmPercent),
                      dutyForPercent(pwmPercent));
        finish("Prueba terminada");
    } else if (pwmPercent == 0) {
        finish("Aun hay movimiento a PWM cero. Umbral de parada no determinado");
    } else {
        startStep(Phase::Descending, pwmPercent - 1);
    }
}

} // namespace

void setup() {
    initBoard();
    initEncoders();
    Serial.begin(115200);
    Serial.println("Zona muerta | Sujeta el robot con las ruedas en el aire");
    Serial.println("1: M1+ | 2: M1- | 3: M2+ | 4: M2- | x: parar");
}

void loop() {
    if (Serial.available() > 0) {
        const int command = Serial.read();
        if (command == 'x' && phase != Phase::Idle) {
            finish("Prueba cancelada");
        } else if (phase == Phase::Idle && command >= '1' && command <= '4') {
            motor = command <= '2' ? Motor::M1 : Motor::M2;
            direction = command == '1' || command == '3' ? 1 : -1;
            trialStartedAt = millis();
            startStep(Phase::Rest, 0);
        }
    }

    if (phase == Phase::Idle) {
        return;
    }

    const uint32_t now = millis();
    if (now - trialStartedAt >= timeoutMs) {
        finish("Tiempo maximo de prueba alcanzado");
        return;
    }

    if (phase == Phase::Rest) {
        const int32_t count = motorCount();
        if (count != windowStartCount) {
            beginWindow(now, count);
        } else if (now - windowStartedAt >= restMs) {
            startStep(Phase::Ascending, pwmPercent + 1);
        }
        return;
    }

    if (now - windowStartedAt >= windowMs) {
        const int32_t count = motorCount();
        const int32_t delta = count - windowStartCount;
        if (secondWindow) {
            checkStep(delta);
        } else {
            firstDelta = delta;
            secondWindow = true;
            beginWindow(now, count);
        }
    }
}
