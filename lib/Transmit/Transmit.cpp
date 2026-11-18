#include "Transmit.h"
#include <WiFi.h>
#include <HTTPClient.h>

namespace Transmit
{
    bool sendData(const char* ssid, const char* password, const char* serverUrl, const String& payload)
    {
        WiFi.begin(ssid, password);

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 25) {
            delay(500);
            Serial.print(".");
            attempts++;
        }

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println(" Failed to connect.");
            return false;
        }

        Serial.println(" Connected! Sending data...");

        HTTPClient http;
        http.begin(serverUrl);
        http.addHeader("Content-Type", "application/json");

        int httpResponseCode = http.POST(payload);

        bool success = false;
        if (httpResponseCode > 0) {
            Serial.printf("[Transmit] HTTP Response code: %d\n", httpResponseCode);
            String response = http.getString();
            Serial.println(response);
            success = (httpResponseCode >= 200 && httpResponseCode < 300);
        } else {
            Serial.printf("[Transmit] Error on HTTP POST: %s\n", http.errorToString(httpResponseCode).c_str());
        }

        http.end();

        // Disconnect and turn off Wi-Fi to save battery in light sleep
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);

        return success;
    }
}