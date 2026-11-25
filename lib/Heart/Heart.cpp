#include <MAX30105.h>
#include <Debug.h>
#include <Heart.h>
#include <Biquad.h>
#include <Accel.h>

namespace
{
    MAX30105 sensor; // actually a MAX30102
    bool foundSensor = false;

    constexpr uint8_t MODE_MULTILED = 0x07; // MODE_CONFIG: multi-LED mode
    constexpr uint8_t SLOT_IR_LED = 0x02;   // SLOTx: LED2 (IR)
    constexpr uint8_t MAX30105_OVF_COUNTER = 0x05;
    constexpr uint8_t MAX30105_FIFODATA    = 0x07;
    constexpr int kFifoDepth = 32;           // samples the chip FIFO holds
    constexpr uint8_t kIntAFullMask = 0x80;      // Interrupt Status 1, bit 7: FIFO almost full
    constexpr uint32_t kAdcMax = 0x3FFFF;    // 18-bit full scale (262143)

    constexpr unsigned long kThreshold = 120000;

    constexpr float kSampleRateHz = 50.0f;
    constexpr uint32_t kSamplePeriodMs = (uint32_t)(1000.0f / kSampleRateHz); // 20 ms
    uint32_t sampleMs = 0; // estimated millis() when the sample being processed was taken (debug plots)

    constexpr int kBeatWindowMs = 667;
    constexpr int kPeakWindowMs  = 111;
    constexpr float thrBeta  = 0.02f;
    constexpr float zAlpha = 0.004f;
    constexpr float kZMeanClamp = 10.0f; // a sample counts as at most 10x the current zMean
    constexpr uint32_t kZSeedSamples = 2 * (uint32_t)kSampleRateHz; // plain average for the first 2 s

    constexpr int kBeatSamples  = kBeatWindowMs * kSampleRateHz / 1000;
    constexpr int kPeakSamples = (kPeakWindowMs * 50 + 500) / 1000; // rounded up
    constexpr int kHeartWindow  = kBeatSamples > kPeakSamples ? kBeatSamples : kPeakSamples; // = 33
    constexpr int kHalf = kHeartWindow / 2;

    BandPass filter;
    bool needsReset = true;
    float lastFiltered = 0;

    // Debug output: print every Nth filtered sample (100 / 3 = ~33 per second)
    constexpr uint8_t kPrintEvery = 1;
    uint8_t printCounter = 0;

    float bpm = 0;

    float win[kHeartWindow];
    uint8_t wIdx = 0;


    constexpr int kPeakBefore = kPeakSamples / 2;                 // 3
    constexpr int kPeakAfter  = kPeakSamples - 1 - kPeakBefore;   // 2
    static_assert(kHeartWindow % 2 == 1, "beat window should be odd so it has a center sample");
    static_assert(kPeakBefore + 1 <= kHalf, "peak window must fit inside the beat window");

    float maPeakSum = 0; // sum(win[center - kPeakBefore, center + kPeakBefore])
    float maBeatSum = 0; // sum of win
    float zMean = 0;

    // samples processed
    uint32_t n = 0;

    bool inBlock = false;
    int blockLen = 0;
    float blockMax = 0;
    uint32_t blockMaxNum = 0;

    uint32_t lastBeatNum = 0;
    bool haveLastBeat = false;

    // --- BPM: EMA of beat intervals, with outlier rejection ---
    constexpr float kMinIntervalS   = 0.3f;  // 200 BPM
    constexpr float kMaxIntervalS   = 2.0f;  // 30 BPM
    constexpr float kMaxDeviation   = 0.3f;  // reject intervals >30% away from the EMA
    constexpr float kIntervalAlpha  = 0.2f;  // EMA weight (~last 8-10 beats)
    constexpr uint8_t kMaxRejects   = 4;     // this many rejections in a row -> restart EMA
    constexpr uint8_t kMinAccepted  = 5;     // intervals needed before reporting BPM

    float emaInterval = 0;   // seconds; 0 = not started
    uint8_t rejects = 0;     // consecutive rejected intervals
    uint8_t accepted = 0;    // accepted intervals since (re)start, capped at kMinAccepted

    void addInterval(float iv)
    {
        if (iv < kMinIntervalS || iv > kMaxIntervalS) {
            // out of plausible range for heart rate
            return;
        }

        if (emaInterval == 0) {
            emaInterval = iv;
            accepted = 1;
            return;
        }

        if (fabsf(iv - emaInterval) > kMaxDeviation * emaInterval) {
            // probably a missed beat (~2x) or an extra beat (~0.5x)
            if (++rejects >= kMaxRejects) { // EMA itself is probably wrong: restart
                emaInterval = iv;
                accepted = 1;
                rejects = 0;
            }
            return;
        }

        rejects = 0;
        emaInterval += kIntervalAlpha * (iv - emaInterval);
        if (accepted < kMinAccepted) accepted++;
        bpm = 60.0f / emaInterval;
    }

    // Called for an accepted block with its sample number
    void onBeat(uint32_t peakNum)
    {
        if (haveLastBeat) {
            float intervalS = (peakNum - lastBeatNum) / kSampleRateHz;
            addInterval(intervalS);
        }
        lastBeatNum = peakNum;
        haveLastBeat = true;
    }

    // index into win[] with offset
    inline int ringIdx(int base, int offset)
    {
        return ((base + offset) % kHeartWindow + kHeartWindow) % kHeartWindow;
    }

    void resetWindow()
    {
        for (int i = 0; i < kHeartWindow; i++) win[i] = 0;
        wIdx = 0;
        maPeakSum = 0;
        maBeatSum = 0;
        n = 0;
        inBlock = false;
        blockLen = 0;
        haveLastBeat = false;
        emaInterval = 0;
        rejects = 0;
        accepted = 0;
        bpm = 0;
        zMean = 0;
    }

    void process(uint32_t sample)
    {
        // (1) put the sample through the bandpass filter
        // (2) square: "Squaring emphasises the large differences resulting from the systolic wave" (the spike)
        // (3) moving averages

        float y = -filter.process((float)sample);
        lastFiltered = y;
        float z = y > 0 ? y * y : 0;

        #if DEBUG
        if (++printCounter >= kPrintEvery) {
            printCounter = 0;
            // ">name:timestamp_ms:value" so Teleplot spaces samples by when they were
            // taken, not by when the batch was printed
            Serial.printf(">ir:%lu:%.2f\n", (unsigned long)sampleMs, y);
        }
        #endif

        int latest = wIdx;
        maBeatSum += z - win[latest];
        win[latest] = z;
        wIdx = (wIdx + 1) % kHeartWindow;
        n++;

        int center = ringIdx(latest, -kHalf);

        maPeakSum += win[ringIdx(center, kPeakAfter)];
        maPeakSum -= win[ringIdx(center, -kPeakBefore - 1)];

        // recompute sums to wipe out rounding errors
        if (wIdx == 0) {
            maBeatSum = 0;
            for (int i = 0; i < kHeartWindow; i++) maBeatSum += win[i];
            maPeakSum = 0;
            for (int k = -kPeakBefore; k <= kPeakAfter; k++) maPeakSum += win[ringIdx(center, k)];
        }

        if (n <= kZSeedSamples) {
            // first ~2 s after a reset: plain average of everything so far
            // (long enough to always contain a few beats)
            zMean += (z - zMean) / n;
        } else {
            // then an EMA, capping each sample's influence so one motion spike
            // can't inflate zMean (and thr1) for many seconds
            zMean += zAlpha * (fminf(z, kZMeanClamp * zMean) - zMean);
        }

        if (n < (uint32_t)kHeartWindow) {
            return;
        }

        float maPeak = maPeakSum / kPeakSamples;
        float maBeat = maBeatSum / kBeatSamples;
        float zCenter = win[center];        // the sample both averages are centered on
        uint32_t centerNum = n - 1 - kHalf; // its absolute sample number (for beat timing)

        float alpha = thrBeta * zMean;
        float thr1 = maBeat + alpha;
        int thr2 = kPeakSamples;


        if (maPeak > thr1) {
            if (!inBlock) {
                inBlock = true;
                blockLen = 1;
                blockMax = zCenter;
                blockMaxNum = centerNum;
            } else {
                blockLen++;
                if (zCenter > blockMax) {
                    blockMax = zCenter;
                    blockMaxNum = centerNum;
                }
            }
        } else if (inBlock) {
            if (blockLen >= thr2) {
                onBeat(blockMaxNum);
            }
            inBlock = false;
            blockLen = 0;
        }
    }

    // Wait this long with the sensor on skin before measuring:
    // contact (and the raw level) is still changing right after placement.
    constexpr int kSettleSamples = (int)kSampleRateHz; // 1 s
    int settleCount = 0;

    // Per-sample handling: skip bad samples, settle after placement, then process.
    void handleRaw(uint32_t raw)
    {
        // off skin or saturated: clear BPM right away and wait for a good signal
        if (raw < kThreshold || raw >= kAdcMax || Accel::isMoving())
        {
            if (!needsReset) resetWindow(); // stop showing the old BPM
            needsReset = true;
            settleCount = 0;
            return;
        }

        if (needsReset)
        {
            // keep the filter locked to the current level while contact settles,
            // so the rise in raw level doesn't reach the beat detector
            filter.reset((float)raw);
            if (++settleCount < kSettleSamples) return;

            resetWindow();
            needsReset = false;
            settleCount = 0;
        }

        process(raw);
    }
}

namespace Heart
{
    bool setup()
    {
        if (!sensor.begin())
        {
            DEBUG_PRINTLN("MAX3010x not found.");
            return false;
        }

        // all defaults but with ledMode = 1 => activeLEDs = 1
        // then the chip will read samples for one LED at a time rather than 2 or 3
        // we only want to use one LED, the IR
        sensor.setup(0x7F, 2, 1, 100, 411, 16384);

        // Switch to multi-LED mode with only slot 1 active for the IR LED
        sensor.setLEDMode(MODE_MULTILED);
        sensor.disableSlots();
        sensor.enableSlot(1, SLOT_IR_LED);

        sensor.setPulseAmplitudeRed(0);   // not used
        sensor.setPulseAmplitudeGreen(0); // no green LED on the MAX30102
        sensor.clearFIFO();               // discard readings from before mode switch

        sensor.disableDATARDY();      // no interrupt per sample
        sensor.disableALCOVF();       // no ambient-light-overflow interrupt
        sensor.setFIFOAlmostFull(7);  // interrupt with 7 slots left = 25 samples = 500 ms at 50/s
        sensor.enableAFULL();
        sensor.getINT1();             // clear interrupt

        // Elgendi et al. 2013 used 0.5-8Hz, on MAX30102 6Hz is clearer for the weak wrist signal
        filter.setup(0.5f, 6.0f, kSampleRateHz);

        foundSensor = true;
        return true;
    }

    void drainFIFO()
    {
        if (!foundSensor)
        {
            return;
        }

        // this also clears INT
        uint8_t intStatus = sensor.getINT1();
        bool aFull = intStatus & kIntAFullMask; // >= 25 samples were waiting (needs enableAFULL())

        // OVF_COUNTER > 0 means the FIFO filled up and samples were overwritten
        uint8_t ovf = sensor.readRegister8(MAX30105_ADDRESS, MAX30105_OVF_COUNTER);
        uint8_t rd = sensor.getReadPointer();
        uint8_t wr = sensor.getWritePointer();

        int nSamples;
        if (ovf > 0)
        {
            nSamples = kFifoDepth; // full
            needsReset = true;     // time gap: samples were lost
            DEBUG_PRINT("HR FIFO overflow, lost: ");
            DEBUG_PRINTLN(ovf);
        }
        else if (wr == rd)
        {
            // exactly 32 samples available
            nSamples = aFull ? kFifoDepth : 0;
        }
        else
        {
            nSamples = (wr - rd + kFifoDepth) % kFifoDepth;
        }
        if (nSamples == 0)
        {
            return;
        }

        Wire.beginTransmission(MAX30105_ADDRESS);
        Wire.write(MAX30105_FIFODATA);
        uint8_t err = Wire.endTransmission(false);
        if (err)
        {
            DEBUG_PRINT("I2C error (drainFIFO): ");
            DEBUG_PRINTLN(err);
            needsReset = true;
            return;
        }

        // Each sample is 3 bytes (one active slot): want up to 32 * 3 = 96 bytes
        // fits in Wire library's 128 byte receive buffer buffer
        uint8_t nBytes = nSamples * 3;
        uint8_t got = Wire.requestFrom((uint8_t)MAX30105_ADDRESS, nBytes);
        if (got != nBytes)
        {
            DEBUG_PRINT("HR short read: ");
            DEBUG_PRINTLN(got);
            while (Wire.available()) Wire.read(); // discard the partial data
            needsReset = true;
            return;
        }

        // assume newest sample was taken about now, earlier ones a sample period apart
        uint32_t drainMs = millis();

        for (int i = 0; i < nSamples; i++)
        {
            sampleMs = drainMs - (uint32_t)(nSamples - 1 - i) * kSamplePeriodMs;
            // one sample
            uint32_t raw = (uint32_t)Wire.read() << 16;
            raw |= (uint32_t)Wire.read() << 8;
            raw |= (uint32_t)Wire.read();
            raw &= kAdcMax;

            handleRaw(raw);
        }
    }

    uint16_t getBPM() {
        if (Accel::isMoving() || accepted < kMinAccepted) {
            // not ready
            return 0;
        }
        return (uint16_t)lroundf(bpm);
    }
}