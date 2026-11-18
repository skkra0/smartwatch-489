#include <Arduino.h>

namespace TimeManager
{
    bool setup(const char* ssid, const char* password);
    String getTimeString(void);
    String getDateString(void);
}