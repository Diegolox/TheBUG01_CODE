#include <Arduino.h>
#include "devices/buzzer.h"
#include "pines.h"


void tocarBuzzer(int frecuencia, int duracion){
    tone(PIN_BUZZER, 2000);  // Sonido a 2000 Hz.
    delay(100);               // Duración del sonido en milisegundos.
    noTone(PIN_BUZZER);      // Silencio.
}
