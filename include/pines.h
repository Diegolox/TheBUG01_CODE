#pragma once

// TheBUG01 - ESP32-WROOM-32
// Guardar en include/pins.h e incluir con: #include "pins.h"
// Los valores son numeros GPIO, NO posiciones fisicas del conector.
// Los nombres conservan las etiquetas de las redes del esquematico.

// --- Pulsadores ---
// Pull-up externo: reposo HIGH, pulsado LOW. Configurar como INPUT.
constexpr int PIN_PULSADOR_1 = 36;
constexpr int PIN_PULSADOR_2 = 39;

// --- Encoders ---
// A/B en el nombre identifica el MOTOR; 1/2 identifica la red del esquema.
// Segun los simbolos de los motores: red _1 -> ENC_B; red _2 -> ENC_A.
constexpr int PIN_ENC_A_1 = 35; // Motor A, canal ENC_B
constexpr int PIN_ENC_A_2 = 34; // Motor A, canal ENC_A
constexpr int PIN_ENC_B_1 = 13; // Motor B, canal ENC_B
constexpr int PIN_ENC_B_2 = 27; // Motor B, canal ENC_A

// --- Driver de motores ---
// Las redes IN_A_1 e IN_A_2 llegan cruzadas respecto a INA_1 e INA_2.
constexpr int PIN_IN_A_1 = 32; // Red IN_A_1 -> entrada INA_2 del driver
constexpr int PIN_IN_A_2 = 33; // Red IN_A_2 -> entrada INA_1 del driver
constexpr int PIN_PWM_A  = 25; // PWM del motor A
constexpr int PIN_IN_B_1 = 26; // Red IN_B_1 -> entrada INB_1 del driver
constexpr int PIN_IN_B_2 = 16; // Red IN_B_2 -> entrada INB_2 del driver
constexpr int PIN_PWM_B  = 14; // PWM del motor B
// STDBY esta conectado a 3V3 en la placa: no tiene GPIO de control.

// --- I2C: BNO055 y tres sensores ToF en el mismo bus ---
constexpr int PIN_I2C_SDA = 21;
constexpr int PIN_I2C_SCL = 22;
// Inicializacion con Arduino: Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

// --- Sensores ToF: control SHUT/XSHUT ---
// LOW mantiene apagado el sensor; permite inicializarlos por separado.
constexpr int PIN_SHUT_LAS_1 = 18; // Sensor ToF 1, conector J5
constexpr int PIN_SHUT_LAS_2 = 17; // Sensor ToF 2, conector J6
constexpr int PIN_SHUT_LAS_3 = 19; // Sensor ToF 3, conector J7

// --- Indicadores ---
constexpr int PIN_LED    = 4;  // LED azul: LOW enciende, HIGH apaga
constexpr int PIN_BUZZER = 23; // Buzzer, terminal positivo conectado al GPIO

// GPIO34, 35, 36 y 39 son solo entradas y no tienen pull-up/pull-down interno.
// Este archivo solo declara constantes; no configura los perifericos.
