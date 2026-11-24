#include <Arduino.h>
#include "devices/encoders.h"
#include "pines.h"

namespace {

struct Encoder {
    int pinA;
    int pinB;
    uint8_t state;
    int32_t count;
};

Encoder motorA{PIN_ENC_A_2, PIN_ENC_A_1, 0, 0};
Encoder motorB{PIN_ENC_B_2, PIN_ENC_B_1, 0, 0};
portMUX_TYPE encoderMux = portMUX_INITIALIZER_UNLOCKED;

// Indice: estado anterior AB seguido del estado nuevo AB.
// La secuencia 00 -> 01 -> 11 -> 10 -> 00 suma cuatro cuentas.
DRAM_ATTR const int8_t quadratureStep[] = {
     0,  1, -1,  0,
    -1,  0,  0,  1,
     1,  0,  0, -1,
     0, -1,  1,  0,
};

uint8_t ARDUINO_ISR_ATTR readChannels(const Encoder &encoder) {
    return (digitalRead(encoder.pinA) << 1) | digitalRead(encoder.pinB);
}

void ARDUINO_ISR_ATTR onEncoderChange(void *context) {
    auto &encoder = *static_cast<Encoder *>(context);
    portENTER_CRITICAL_ISR(&encoderMux);
    const uint8_t next = readChannels(encoder);
    encoder.count += quadratureStep[(encoder.state << 2) | next];
    encoder.state = next;
    portEXIT_CRITICAL_ISR(&encoderMux);
}

void startEncoder(Encoder &encoder) {
    encoder.state = readChannels(encoder);
    attachInterruptArg(encoder.pinA, onEncoderChange, &encoder, CHANGE);
    attachInterruptArg(encoder.pinB, onEncoderChange, &encoder, CHANGE);
}

} // namespace

void initEncoders() {
    startEncoder(motorA);
    startEncoder(motorB);
}

EncoderReadings readEncoders() {
    portENTER_CRITICAL(&encoderMux);
    const EncoderReadings readings{
        {motorA.count, motorA.state},
        {motorB.count, motorB.state},
    };
    portEXIT_CRITICAL(&encoderMux);
    return readings;
}
