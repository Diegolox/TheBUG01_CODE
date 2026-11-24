#pragma once
#include <stdint.h>

struct EncoderReading {
    int32_t count;
    uint8_t state; // Canal A en bit 1, canal B en bit 0.
};

struct EncoderReadings {
    EncoderReading m1;
    EncoderReading m2;
};

// Iniciar una vez en setup(), despues de initBoard().
void initEncoders();

// Devuelve una lectura conjunta de los dos encoders en cuadratura x4.
EncoderReadings readEncoders();
