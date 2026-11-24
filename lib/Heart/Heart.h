#pragma once
#include <Arduino.h>


namespace Heart {
    bool setup();
    void drainFIFO();   // read + process all queued samples; call at least every ~600 ms at 50 samples/s
    uint16_t getBPM();  // 0 = no reading
}
