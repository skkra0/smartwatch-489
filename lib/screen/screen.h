namespace Screen
{
    bool setup(void);

    void writeSteps(uint32_t steps);
    void writeBPM(uint16_t bpm);
    void writeBattery(int battery);
    void writeTime(String& time);
    void writeTime(const char* time);
}