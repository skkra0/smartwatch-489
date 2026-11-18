#include "BatteryMonitor.h"

namespace
{
    uint8_t _adcPin = 0;
    
    // Note: If your hardware uses a 1:1 voltage divider (two equal resistors) 
    // to step down 4.2V to a safe level for the ESP32 ADC, keep multiplier at 2.0f.
    // If connected directly or using an integrated PMU, adjust accordingly.
    const float VOLTAGE_MULTIPLIER = 2.0f; 
    
    const float V_MIN = 3.3f; // Empty / Safe shutdown threshold
    const float V_MAX = 4.2f; // Fully charged LiPo
}

namespace BatteryMonitor
{
    bool setup(uint8_t pin)
    {
        _adcPin = pin;
        pinMode(_adcPin, INPUT);
        
        // Configure ESP32-S3 ADC attenuation for full range reading (~0 to 3.3V)
        analogSetAttenuation(ADC_11db);
        return true;
    }

    float getVoltage(void)
    {
        // Use analogReadMilliVolts for accurate hardware-calibrated mV readings on ESP32
        uint32_t mVolts = analogReadMilliVolts(_adcPin);
        
        // Apply the voltage divider multiplier to get the true battery voltage
        float voltage = (mVolts / 1000.0f) * VOLTAGE_MULTIPLIER;
        return voltage;
    }

    int getPercentage(void)
    {
        float voltage = getVoltage();
        
        if (voltage >= V_MAX) return 100;
        if (voltage <= V_MIN) return 0;
        
        // Linear mapping from V_MIN to V_MAX
        int percent = (int)(((voltage - V_MIN) / (V_MAX - V_MIN)) * 100.0f);
        return percent;
    }

    bool isLow(void)
    {
        // Trigger low battery safety state when voltage dips to 3.4V or lower
        return getVoltage() <= 3.4f;
    }
}