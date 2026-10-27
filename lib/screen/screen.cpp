#include <Arduino.h>
#include <Debug.h>
#include <SPI.h>
#include <TFT_eSPI.h>

namespace
{
  TFT_eSPI tft = TFT_eSPI();
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

  void writeStringToScreen(String &string)
  {
    tft.drawString(string, 120, 120, 1);
  }
}