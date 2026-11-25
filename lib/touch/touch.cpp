#include <CST816S.h>
#include <Debug.h>
#include <Wire.h>

namespace
{
  CST816S touch(TP_SDA, TP_SCL, TP_RST, TP_INT);
}

namespace Touch
{
  bool setup(void)
  {
    touch.begin();
    
    // test i2c connection
    Wire.beginTransmission(CST816S_ADDRESS);
    uint8_t err = Wire.endTransmission();
    if (err) {
      DEBUG_PRINT("CST816S not responding on i2c, err = ");
      DEBUG_PRINTLN(err);
      return false;
    }
    return true;
  }

  bool checkTouch(void)
  {
    if (touch.available()) {
      return true;
    }
    return false;
  }
}