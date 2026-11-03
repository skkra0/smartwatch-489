#pragma once

#include <stdint.h>

namespace BLE {
    void setup();
    // store the latest values; they're sent on the next send()
    void updateSteps(uint32_t steps);
    void updateHeart(uint8_t bpm);
    // notify subscribers with all current values in one packet
    void send();
}
