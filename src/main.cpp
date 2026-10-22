#include <Wire.h>
#include <Arduino.h>
#include <Accel.h>
#include <Debug.h>

#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

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

  if (setupAccelFIFO()) {
    DEBUG_PRINTLN("setup fifo");
    pinMode(18, INPUT);
    gpio_wakeup_enable(GPIO_NUM_18, GPIO_INTR_HIGH_LEVEL);
    esp_sleep_enable_gpio_wakeup();
  }

  //screen setup
  tft.init();
  Serial.println("Init completed.");
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  // Draw some test graphics
  //set background to blue
  tft.fillCircle(120, 120, 110, TFT_BLUE);
  //set text color to white with blue background
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  //set text datum to middle center
  tft.setTextDatum(MC_DATUM);
  //draw string in the middle of the screen
  tft.drawString("OK :3", 120, 120, 1);

  // 1. Handle Touch Reset Pin first to wake up the controller
  pinMode(TP_RST, OUTPUT);
  digitalWrite(TP_RST, LOW);
  delay(10);
  digitalWrite(TP_RST, HIGH); // Pull high to take it out of reset
  delay(50);

  // 3. Configure your interrupt pin (if your touch library needs it)
  pinMode(TP_INT, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(18)) {
    drainFIFO();
  }

  esp_light_sleep_start(); 
}