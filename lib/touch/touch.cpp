#include <CST816S.h>

namespace
{
  CST816S touch(TP_SDA, TP_SCL, TP_RST, TP_INT);
}

namespace Touch
{
  bool setup(void)
  {
    touch.begin();
    return true;
  }

  bool checktouch(void)
  {
    if (touch.available()) {
      return true;
    }
    return false;
  }
}