#ifndef TRANSMIT_H
#define TRANSMIT_H

#include <Arduino.h>

namespace Transmit
{
    // Connects to Wi-Fi, sends an HTTP POST request with a payload, and disconnects
    bool sendData(const char* ssid, const char* password, const char* serverUrl, const String& payload);
}

#endif