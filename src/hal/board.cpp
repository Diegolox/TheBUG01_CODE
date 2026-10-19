#include <Arduino.h>
#include "pins.h"
#include "hal/board.h"

void initBoard() {
    // Primero deshabilitar la excitacion de los motores.
    // STDBY esta conectado permanentemente a 3V3.
    pinMode(PIN_PWM_A, OUTPUT);
    digitalWrite(PIN_PWM_A, LOW);

    pinMode(PIN_PWM_B, OUTPUT);
    digitalWrite(PIN_PWM_B, LOW);

    // Entradas de direccion del driver.
    pinMode(PIN_IN_A_1, OUTPUT);
    digitalWrite(PIN_IN_A_1, LOW);

    pinMode(PIN_IN_A_2, OUTPUT);
    digitalWrite(PIN_IN_A_2, LOW);

    pinMode(PIN_IN_B_1, OUTPUT);
    digitalWrite(PIN_IN_B_1, LOW);

    pinMode(PIN_IN_B_2, OUTPUT);
    digitalWrite(PIN_IN_B_2, LOW);

    // Mantener los tres ToF apagados hasta inicializarlos
    // individualmente y asignar sus direcciones I2C.
    pinMode(PIN_SHUT_LAS_1, OUTPUT);
    digitalWrite(PIN_SHUT_LAS_1, LOW);

    pinMode(PIN_SHUT_LAS_2, OUTPUT);
    digitalWrite(PIN_SHUT_LAS_2, LOW);

    pinMode(PIN_SHUT_LAS_3, OUTPUT);
    digitalWrite(PIN_SHUT_LAS_3, LOW);

    // Buzzer sin excitacion.
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);

    // LED activo en LOW.
    // Revisar alimentacion a +5V de esta rama del circuito.
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH);

    // Pulsadores con pull-up externa.
    pinMode(PIN_PULSADOR_1, INPUT);
    pinMode(PIN_PULSADOR_2, INPUT);

    // Encoders: asumir salidas push-pull a 3V3.
    pinMode(PIN_ENC_A_1, INPUT);
    pinMode(PIN_ENC_A_2, INPUT);
    pinMode(PIN_ENC_B_1, INPUT);
    pinMode(PIN_ENC_B_2, INPUT);

    // El bus I2C se inicializa en el modulo correspondiente:
    // Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
}