#include <Arduino.h>
#include <Debug.h>
#include <SPI.h>
#include <TFT_eSPI.h>

#include "Screen.h"

namespace
{
  TFT_eSPI tft = TFT_eSPI();

  template <typename T>
  inline void writeStringToScreen(T string, int32_t x, int32_t y, uint8_t size, const char* widest = nullptr)
  {
    tft.setTextPadding(widest ? tft.textWidth(widest, size) : 0); // measure in the same font that is drawn
    tft.drawString(string, x, y, size);
    tft.setTextPadding(0);
  }
}

namespace Screen
{
  bool setup(void)
  {
    // screen setup
    tft.init();
    DEBUG_PRINTLN("Init completed.");
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);

    // Draw some test graphics
    // set background to blue
    tft.fillCircle(120, 120, 110, TFT_BLUE);
    // set text color to white with blue background
    tft.setTextColor(TFT_WHITE, TFT_BLUE);
    // set text datum to middle center
    tft.setTextDatum(MC_DATUM);
    // draw string in the middle of the screen
    //  tft.drawString("OK :3", 120, 120, 1);

    // 1. Handle Touch Reset Pin first to wake up the controller
    pinMode(TP_RST, OUTPUT);
    digitalWrite(TP_RST, LOW);
    delay(10);
    digitalWrite(TP_RST, HIGH); // Pull high to take it out of reset
    delay(50);

    // 3. Configure your interrupt pin (if your touch library needs it)
    pinMode(TP_INT, INPUT_PULLUP);

    // todo: add actual error checking to this
    return true;
  }

  void writeSteps(uint32_t steps) {
    writeStringToScreen("Steps", 75, 120, 2);
    writeStringToScreen(String(steps), 75, 140, 4);
  }

  void writeBPM(uint16_t bpm) {
    writeStringToScreen("<3 rate", 170, 120, 2); 
    writeStringToScreen(bpm > 0 ? String(bpm) : "--", 170, 140, 4, "888");
  }

  void writeBattery(int battery) {
    writeStringToScreen("Battery", 120, 160, 2);
    writeStringToScreen(String(battery) + "%", 120, 180, 4);
  }

  void writeTime(String& time) {
    writeStringToScreen(time, 120, 80, 4);
  }
  
  void writeTime(const char* time) {
    writeStringToScreen(time, 120, 80, 4);
  }
}