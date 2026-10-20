#include <Arduino.h>
#include <hal/board.h>
#include <devices/buzzer.h>


void setup() {
  initBoard();
}

void loop() {
  tocarBuzzer(2000, 100);
  delay(2000);
}