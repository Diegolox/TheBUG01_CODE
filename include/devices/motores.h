#pragma once
#include <stdint.h>

enum class Motor : uint8_t { M1 = 1, M2 = 2 };

// Llamar despues de initBoard(). PWM entre -255 y 255, con 0 para parar.
void setMotorPwm(Motor motor, int duty);
