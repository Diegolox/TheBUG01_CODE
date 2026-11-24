#include <Arduino.h>
#include "devices/motores.h"
#include "hal/board.h"

namespace {

constexpr unsigned pwmPercent = 15;
constexpr int pwmDuty = (255 * pwmPercent + 50) / 100;
constexpr uint32_t pulseMs = 1000;

bool moving = false;
uint32_t startedAt = 0;

void driveBoth(int duty) {
    setMotorPwm(Motor::M1, duty);
    setMotorPwm(Motor::M2, duty);
}

void stopMovement() {
    driveBoth(0);
    moving = false;
    Serial.println("Parado");
}

} // namespace

void setup() {
    initBoard();
    Serial.begin(115200);
    Serial.printf("Movimiento | a: adelante | r: retroceso | x: parar | %u%% durante %lu ms\n",
                  pwmPercent, static_cast<unsigned long>(pulseMs));
}

void loop() {
    if (moving && millis() - startedAt >= pulseMs) {
        stopMovement();
    }

    if (Serial.available() == 0) {
        return;
    }

    const int command = Serial.read();
    if (command == 'x' && moving) {
        stopMovement();
    } else if (!moving && (command == 'a' || command == 'r')) {
        const bool forward = command == 'a';

        // En el robot montado, el avance fisico corresponde a PWM negativo en ambas ruedas.
        startedAt = millis();
        moving = true;
        driveBoth(forward ? -pwmDuty : pwmDuty);
        Serial.println(forward ? "Adelante" : "Retroceso");
    }
}
