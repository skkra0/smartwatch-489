#include <Wire.h>
#include <Arduino.h>
#include <Accel.h>
#include <Debug.h>

void setup() {
  Wire.begin();
  #if DEBUG
    Serial.begin(115200);
  #endif

  if (setupAccelFIFO()) {
    DEBUG_PRINTLN("setup fifo");
    pinMode(18, INPUT);
    gpio_wakeup_enable(GPIO_NUM_18, GPIO_INTR_HIGH_LEVEL);
    esp_sleep_enable_gpio_wakeup();
  }
}

void loop() {
  if (digitalRead(18)) {
    drainFIFO();
  }

  esp_light_sleep_start(); 
}