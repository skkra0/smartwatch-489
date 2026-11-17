#ifndef FLASH_ACCESS_H
#define FLASH_ACCESS_H

#include <Arduino.h>

namespace FlashAccess
{
    void setup();
    void saveSteps(uint32_t steps);
    uint32_t loadSteps();
    void saveHeartRateSample(uint8_t heartRate);
}

#endif