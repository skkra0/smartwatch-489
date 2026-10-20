#pragma once

#define ACCEL_COUNTS_PER_G    16384.0f // LIS3DH gives 16-bit signed accelerations. We configure it for ±2 g. Then 16384 = 1g
#define ACCEL_SAMPLE_HZ       50
#define ACCEL_FILTER_ORDER    4 // number of samples in the moving average filter
#define ACCEL_THRESH_ORDER    4 // number of recent midpoints to use for the dynamic threshold
#define ACCEL_WINDOW             (ACCEL_FILTER_ORDER * 4 + 1)  // number of samples to search for a peak. 17 samples = 0.34 s
#define ACCEL_HALF            (ACCEL_WINDOW / 2)                
#define ACCEL_SENS            0.1f                    // sensitivity in g. Count a step when max and min above and below the threshold and amplitude is higher than this
#define ACCEL_MIN_SEARCH      ACCEL_SAMPLE_HZ               // 1 s to find the min after a max
#define ACCEL_REG_OFF_TIME    (2 * ACCEL_SAMPLE_HZ)         // number of samples without a step to leave walking mode
#define ACCEL_STEPS_TO_START  8                       // number of steps before we start counting
#define ACCEL_MAX_FAILS       2                       // consecutive failed pairs before reset

enum State {
    FIND_MIN,
    FIND_MAX,
};

void handleAccelFIFOInterrupt(void);
void setupAccelFIFO(void);