#include <Arduino.h>
#include "devices/motores.h"
#include "pines.h"

namespace {

struct MotorOutput {
    int pwm;
    int in1;
    int in2;
    int direction;
};

// Las etiquetas IN_A_1 e IN_A_2 estan cruzadas respecto al driver.
MotorOutput motor1{PIN_PWM_A, PIN_IN_A_2, PIN_IN_A_1, 0};
MotorOutput motor2{PIN_PWM_B, PIN_IN_B_1, PIN_IN_B_2, 0};

} // namespace

void setMotorPwm(Motor motor, int duty) {
    MotorOutput &output = motor == Motor::M1 ? motor1 : motor2;
    const int boundedDuty = constrain(duty, -255, 255);
    const int direction = (boundedDuty > 0) - (boundedDuty < 0);

    if (direction != output.direction) {
        // Quitar el PWM antes de cambiar las entradas de direccion.
        analogWrite(output.pwm, 0);
        digitalWrite(output.in1, direction > 0 ? HIGH : LOW);
        digitalWrite(output.in2, direction < 0 ? HIGH : LOW);
        output.direction = direction;
    }

    analogWrite(output.pwm, abs(boundedDuty));
}
