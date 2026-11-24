#include <Arduino.h>
#include "devices/encoders.h"
#include "hal/board.h"

void setup() {
    initBoard();
    Serial.begin(115200);
    initEncoders();
    Serial.println("TheBUG01 | Encoders x4 | Gira una rueda a mano");
}

void loop() {
    const EncoderReadings readings = readEncoders();
    const EncoderReading &m1 = readings.m1;
    const EncoderReading &m2 = readings.m2;

    Serial.printf("M1: %ld (AB=%u%u) | M2: %ld (AB=%u%u)\n",
                  static_cast<long>(m1.count), unsigned(m1.state >> 1), unsigned(m1.state & 1),
                  static_cast<long>(m2.count), unsigned(m2.state >> 1), unsigned(m2.state & 1));
    delay(100);
}
