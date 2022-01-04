#include <Arduino.h>

void setup() {
  pinMode(PB_1, OUTPUT);
}

void loop() {
  digitalWrite(PB_1, HIGH);
  delay(1000);
  digitalWrite(PB_1, LOW);
  delay(1000);
}