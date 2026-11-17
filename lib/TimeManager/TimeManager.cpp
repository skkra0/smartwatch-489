#include "TimeManager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

namespace
{
    const char* ntpServer = "pool.ntp.org";
    const long  gmtOffset_sec = -18000;    // Adjust to your timezone offset in seconds (e.g., -18000 for EST)
    const int   daylightOffset_sec = 3600; // Daylight saving offset in seconds (3600 if 1 hour, 0 if none)
}

namespace TimeManager
{
    bool setup(const char* ssid, const char* password)
    {
        WiFi.begin(ssid, password);
    
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 25) {
            delay(500);
            attempts++;
        }
    
        if (WiFi.status() == WL_CONNECTED) {
            configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        
            struct tm timeinfo;
            int syncAttempts = 0;
            while (!getLocalTime(&timeinfo) && syncAttempts < 10) {
                delay(500);
                syncAttempts++;
            }
        
        
            // Always clean up Wi-Fi whether NTP sync succeeded or failed
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
        
            // Return true only if NTP sync actually succeeded
            return (syncAttempts < 10);
        }    
        return false;   
    }
    
    String getTimeString(void)
    {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
            return "--:--";
        }
        char buffer[10];
        strftime(buffer, sizeof(buffer), "%H:%M", &timeinfo);
        return String(buffer);
    }

    String getDateString(void)
    {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
            return "---";
        }
        char buffer[15];
        strftime(buffer, sizeof(buffer), "%b %d", &timeinfo);
        return String(buffer);
    }
}