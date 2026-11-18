#include "FlashAccess.h"
#include <Preferences.h>

namespace
{
    Preferences prefs;
}

namespace FlashAccess
{
    void setup()
    {
        // Open NVS namespace "wearable" in read-write mode (false)
        prefs.begin("wearable", false);
    }

    void saveSteps(uint32_t steps)
    {
        prefs.putUInt("steps", steps);
    }

    uint32_t loadSteps()
    {
        // Returns 0 if no step record exists yet
        return prefs.getUInt("steps", 0);
    }

    void saveHeartRateSample(uint8_t heartRate)
    {
        // Keep a rolling index to store up to 288 samples (24 hours at 5-min intervals)
        unsigned int count = prefs.getUInt("hr_count", 0);
        char key[15];
        snprintf(key, sizeof(key), "hr_%u", count % 288);
        
        prefs.putUChar(key, heartRate);
        prefs.putUInt("hr_count", count + 1);
    }
}