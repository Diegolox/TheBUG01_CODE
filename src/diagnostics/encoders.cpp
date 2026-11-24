#include <Arduino.h>
#include "hal/board.h"
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

void setup() {
    initBoard();
    Serial.begin(115200);

    startEncoder(motorA);
    startEncoder(motorB);
    Serial.println("TheBUG01 | Encoders x4 | Gira una rueda a mano");
}

void loop() {
    portENTER_CRITICAL(&encoderMux);
    const int32_t countA = motorA.count;
    const int32_t countB = motorB.count;
    const uint8_t stateA = motorA.state;
    const uint8_t stateB = motorB.state;
    portEXIT_CRITICAL(&encoderMux);

    Serial.printf("M1: %ld (AB=%u%u) | M2: %ld (AB=%u%u)\n",
                  static_cast<long>(countA), unsigned(stateA >> 1), unsigned(stateA & 1),
                  static_cast<long>(countB), unsigned(stateB >> 1), unsigned(stateB & 1));
    delay(100);
}
