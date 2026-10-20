#include <Wire.h>
#include <Arduino.h>
#include <Accel.h>
#include <Debug.h>

void setup() {
  Wire.begin();
  #if DEBUG
    Serial.begin(115200);
  #endif

  setupAccelFIFO();
  attachInterrupt(digitalPinToInterrupt(18), handleAccelFIFOInterrupt, RISING);
}

void loop() {
  sleep(600);
}