#include <Wire.h>
#include <Arduino.h>
#include <Accel.h>
#include <Debug.h>
#include <Screen.h>
#include <Touch.h>
#include <Heart.h>
#include <TimeManager.h>
#include <Transmit.h>
#include <BatteryMonitor.h>
#include <FlashAccess.h>

uint32_t steps = 0;
uint16_t bpm = 0;
int battery = 0;

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
    pinMode(41, INPUT);
    gpio_wakeup_enable(GPIO_NUM_41, GPIO_INTR_HIGH_LEVEL);
    esp_sleep_enable_gpio_wakeup();
  }

  if (Screen::setup()) {
    DEBUG_PRINTLN("Screen is ready");
  }

  if (Touch::setup()) {
    DEBUG_PRINTLN("Touch is ready");
  }
  
  // //using eileen's hostpot for now
  // if (TimeManager::setup("Eileen's Hotspot", "eileenhotspot")) {
  //   DEBUG_PRINTLN("TimeManager is ready");
  // }

  // if (BatteryMonitor::setup(6)) {
  //   DEBUG_PRINTLN("BatteryMonitor is ready");
  // }

  if (Heart::setup()) {
    DEBUG_PRINTLN("heart ok");
    // MAX30102 INT is active-low: needs pull-up to keep it high normally
    pinMode(42, INPUT_PULLUP);
    gpio_wakeup_enable(GPIO_NUM_42, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
  }

  //String text = TimeManager::getTimeString();
  Screen::writeTime("TI:ME");
  Screen::writeBPM(bpm);
  Screen::writeSteps(steps);

  // battery = BatteryMonitor::getPercentage();
  Screen::writeBattery(battery);
}

void loop() {
  Accel::drainFIFO();
  Heart::drainFIFO(); // todo: wire particle sensor interrupt

  uint32_t newSteps = Accel::getSteps();
  if (newSteps != steps) {
    steps = newSteps;
    Screen::writeSteps(newSteps);
  }

  uint16_t newBPM = Heart::getBPM();
  if (newBPM != bpm) {
    bpm = newBPM;
    Screen::writeBPM(newBPM); 
  }

  // todo: update time if time changed

  // int newBat = BatteryMonitor::getPercentage();
  // if (newBat != battery) {
  //   battery = newBat;
  //   Screen::writeBattery(newBat);
  // }

  if (Touch::checkTouch()) {
    DEBUG_PRINTLN("touched");
  }

  Serial.flush();                                    // finish debug output before sleeping
  esp_light_sleep_start();
}