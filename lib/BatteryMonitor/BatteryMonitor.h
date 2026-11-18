#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <Arduino.h>

namespace BatteryMonitor
{
    // Initialize the battery monitor with the specified ADC pin
    bool setup(uint8_t pin);
    
    // Returns the calculated battery voltage in volts (e.g., 3.85)
    float getVoltage(void);
    
    // Returns the battery level as a percentage (0 to 100)
    int getPercentage(void);
    
    // Returns true if the voltage drops below a safe threshold (e.g., <= 3.4V)
    bool isLow(void);
}

#endif