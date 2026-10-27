#include <Wire.h>
#include <Arduino.h>
#include <Accel.h>
#include <Debug.h>
#include <Screen.h>
#include <CST816S.h>

#include <SPI.h>
#include <TFT_eSPI.h>

CST816S touch(TP_SDA, TP_SCL, TP_RST, TP_INT);

void i2cBusClear(int sda, int scl) {
    digitalWrite(scl, HIGH);             // preset levels so switching to output doesn't glitch low
    digitalWrite(sda, HIGH);
    pinMode(sda, INPUT_PULLUP);
    pinMode(scl, OUTPUT_OPEN_DRAIN);
    for (int i = 0; i < 9 && !digitalRead(sda); i++) {
      digitalWrite(scl, LOW);  delayMicroseconds(5);
      digitalWrite(scl, HIGH); delayMicroseconds(5);
    }
    pinMode(sda, OUTPUT_OPEN_DRAIN);
    digitalWrite(sda, LOW);  delayMicroseconds(5);   // START (SCL is high)
    digitalWrite(sda, HIGH); delayMicroseconds(5);   // STOP
  }

void setup() {
  #if DEBUG
    Serial.begin(115200);
    delay(1000);
  #endif

  i2cBusClear(TP_SDA, TP_SCL);
  // 2. Initialize the I2C bus on the correct pins (moving it away from TFT_DC on Pin 9)
  if (!Wire.begin(TP_SDA, TP_SCL)) {
    DEBUG_PRINTLN("i2c init failed");
  }

  if (Accel::setup()) {
    DEBUG_PRINTLN("setup fifo");
    pinMode(18, INPUT);
    gpio_wakeup_enable(GPIO_NUM_18, GPIO_INTR_HIGH_LEVEL);
    esp_sleep_enable_gpio_wakeup();
  }

  if (Screen::setup()) {
    DEBUG_PRINTLN("setup screen");
  }

  touch.begin();
  
  Serial.println("Touch test ready. Touch the screen/pad.");
  String steps = String(Accel::getSteps());
  Screen::writeStringToScreen(steps, 120, 120, 4);
}

void loop() {
  if (digitalRead(18)) {
    Accel::drainFIFO();
    String steps = String(Accel::getSteps());
    Screen::writeStringToScreen(steps, 120, 120, 4);
  }
  if (touch.available()) {
    Serial.println("touched");
  }
  //esp_light_sleep_start(); 

}