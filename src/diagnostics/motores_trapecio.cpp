#include <Arduino.h>
#include "hal/board.h"
#include "devices/motores.h"

namespace {

constexpr uint32_t rampUpMs = 2000;
constexpr uint32_t holdMs = 1000;
constexpr uint32_t rampDownMs = 2000;
constexpr uint32_t rampDownStartMs = rampUpMs + holdMs;
constexpr uint32_t trialDurationMs = rampDownStartMs + rampDownMs;
constexpr uint32_t pwmUpdateMs = 10;
constexpr uint32_t reportMs = 100;
constexpr uint8_t maxDutyPercent = 60;
constexpr uint16_t maxDuty = (255u * maxDutyPercent + 50u) / 100u;
static_assert(maxDutyPercent > 0 && maxDutyPercent <= 100, "PWM maximo fuera de rango");

Motor activeMotor = Motor::M1;
bool trialActive = false;
uint32_t startedAt = 0;
uint32_t lastUpdateAt = 0;
uint32_t lastReportAt = 0;

uint8_t trapezoidDuty(uint32_t elapsed) {
    if (elapsed < rampUpMs) {
        return maxDuty * elapsed / rampUpMs;
    }
    if (elapsed < rampDownStartMs) {
        return maxDuty;
    }
    if (elapsed < trialDurationMs) {
        return maxDuty * (trialDurationMs - elapsed) / rampDownMs;
    }
    return 0;
}

void report(Motor motor, uint32_t elapsed, uint8_t duty) {
    Serial.printf("M%d | t=%lu ms | PWM=%.1f%%\n",
                  static_cast<int>(motor), static_cast<unsigned long>(elapsed), duty * 100.0 / 255.0);
}

void startTrial(Motor motor) {
    activeMotor = motor;
    trialActive = true;
    startedAt = millis();
    lastUpdateAt = startedAt;
    lastReportAt = startedAt;
    report(motor, 0, 0);
}

void stopTrial(uint32_t elapsed, const char *message) {
    setMotorPwm(activeMotor, 0);
    trialActive = false;

    report(activeMotor, elapsed, 0);
    Serial.println(message);
}

} // namespace

void setup() {
    initBoard();
    Serial.begin(115200);
    Serial.printf("TheBUG01 | 1: M1 | 2: M2 | x: parar | PWM maximo: %u%%\n",
                  static_cast<unsigned>(maxDutyPercent));
}

void loop() {
    while (Serial.available() > 0) {
        const int command = Serial.read();
        if (command == 'x' && trialActive) {
            stopTrial(millis() - startedAt, "Prueba cancelada");
            return;
        }

        if (!trialActive && (command == '1' || command == '2')) {
            startTrial(command == '1' ? Motor::M1 : Motor::M2);
        }
    }

    if (!trialActive) {
        return;
    }

    const uint32_t now = millis();
    const uint32_t elapsed = now - startedAt;
    if (elapsed >= trialDurationMs) {
        stopTrial(elapsed, "Prueba terminada");
        return;
    }

    if (now - lastUpdateAt >= pwmUpdateMs) {
        const uint8_t duty = trapezoidDuty(elapsed);
        setMotorPwm(activeMotor, duty);
        lastUpdateAt = now;

        if (now - lastReportAt >= reportMs) {
            report(activeMotor, elapsed, duty);
            lastReportAt = now;
        }
    }
}
