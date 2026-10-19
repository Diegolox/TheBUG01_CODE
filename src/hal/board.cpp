#include <Arduino.h>
#include "pins.h"
#include "hal/board.h"

void initPlaca() {
    // Pulsadores: tienen pull-up externo.
    pinMode(PIN_PULSADOR_1, INPUT);
    pinMode(PIN_PULSADOR_2, INPUT);

    // Encoders.
    pinMode(PIN_ENC_A_1, INPUT);
    pinMode(PIN_ENC_A_2, INPUT);
    pinMode(PIN_ENC_B_1, INPUT);
    pinMode(PIN_ENC_B_2, INPUT);

    // Precargar los niveles antes de habilitar las salidas.
    digitalWrite(PIN_PWM_A, LOW);
    digitalWrite(PIN_PWM_B, LOW);

    digitalWrite(PIN_IN_A_1, LOW);
    digitalWrite(PIN_IN_A_2, LOW);
    digitalWrite(PIN_IN_B_1, LOW);
    digitalWrite(PIN_IN_B_2, LOW);

    digitalWrite(PIN_LED, HIGH);   // LED apagado.
    digitalWrite(PIN_BUZZER, LOW);

    digitalWrite(PIN_SHUT_LAS_1, LOW);
    digitalWrite(PIN_SHUT_LAS_2, LOW);
    digitalWrite(PIN_SHUT_LAS_3, LOW);

    pinMode(PIN_PWM_A, OUTPUT);
    pinMode(PIN_PWM_B, OUTPUT);

    pinMode(PIN_IN_A_1, OUTPUT);
    pinMode(PIN_IN_A_2, OUTPUT);
    pinMode(PIN_IN_B_1, OUTPUT);
    pinMode(PIN_IN_B_2, OUTPUT);

    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_BUZZER, OUTPUT);

    // Los ToF quedan apagados hasta su inicialización.
    pinMode(PIN_SHUT_LAS_1, OUTPUT);
    pinMode(PIN_SHUT_LAS_2, OUTPUT);
    pinMode(PIN_SHUT_LAS_3, OUTPUT);
}