#include <Arduino.h>
#include "hal/board.h"
#include "pines.h"

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

struct Motor {
    int number;
    int pwm;
    int in1;
    int in2;
};

// El esquema cruza las etiquetas IN_A_1 e IN_A_2 respecto al driver.
constexpr Motor motor1{1, PIN_PWM_A, PIN_IN_A_2, PIN_IN_A_1};
constexpr Motor motor2{2, PIN_PWM_B, PIN_IN_B_1, PIN_IN_B_2};

const Motor *activeMotor = nullptr;
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

void report(const Motor &motor, uint32_t elapsed, uint8_t duty) {
    Serial.printf("M%d | t=%lu ms | PWM=%.1f%%\n",
                  motor.number, static_cast<unsigned long>(elapsed), duty * 100.0 / 255.0);
}

void startTrial(const Motor &motor) {
    analogWrite(motor.pwm, 0);
    digitalWrite(motor.in2, LOW);
    digitalWrite(motor.in1, HIGH);

    activeMotor = &motor;
    startedAt = millis();
    lastUpdateAt = startedAt;
    lastReportAt = startedAt;
    report(motor, 0, 0);
}

void stopTrial(uint32_t elapsed, const char *message) {
    const Motor &motor = *activeMotor;
    analogWrite(motor.pwm, 0);
    digitalWrite(motor.in1, LOW);
    digitalWrite(motor.in2, LOW);
    activeMotor = nullptr;

    report(motor, elapsed, 0);
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
        if (command == 'x' && activeMotor) {
            stopTrial(millis() - startedAt, "Prueba cancelada");
            return;
        }

        if (!activeMotor && (command == '1' || command == '2')) {
            startTrial(command == '1' ? motor1 : motor2);
        }
    }

    if (!activeMotor) {
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
        analogWrite(activeMotor->pwm, duty);
        lastUpdateAt = now;

        if (now - lastReportAt >= reportMs) {
            report(*activeMotor, elapsed, duty);
            lastReportAt = now;
        }
    }
}
