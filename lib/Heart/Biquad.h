#pragma once
#include <math.h>

// Second-order filter
//   y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
struct Biquad {
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float x1 = 0, x2 = 0;                         // x[n-1], x[n-2]
    float y1 = 0, y2 = 0;                         // y[n-1], y[n-2]

    // Butterworth (Q = 1/sqrt(2)) filter
    // Formulas from the "Audio EQ Cookbook" (R. Bristow-Johnson).
    // @param highpass: true for highpass and false for lowpass 
    // @param fc: cutoff frequency
    // @param fs: sampling frequency
    void setup(bool highpass, float fc, float fs) {
        const float Q = 0.70710678f; // quality factor
        float w0 = 2.0f * (float)M_PI * fc / fs;
        float c = cosf(w0);
        float alpha = sinf(w0) / (2.0f * Q);
        float a0 = 1.0f + alpha;

        if (highpass) {
            b0 = (1.0f + c) / 2.0f;
            b1 = -(1.0f + c);
        } else {
            b0 = (1.0f - c) / 2.0f;
            b1 = (1.0f - c);
        }
        b2 = b0;
        a1 = -2.0f * c;
        a2 = 1.0f - alpha;

        b0 /= a0; b1 /= a0; b2 /= a0; a1 /= a0; a2 /= a0;
    }

    // Set the state as if the input had been constant at x0
    void reset(float x0) {
        // y = b0x + b1x + b2x - a1y - a2y
        float coeff = (b0 + b1 + b2) / (1.0f + a1 + a2);
        float y0 = x0 * coeff;
        x1 = x2 = x0;
        y1 = y2 = y0;
    }

    float process(float x) {
        float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1;
        x1 = x;
        y2 = y1;
        y1 = y;
        return y;
    }
};

// bandpass filter: butterworth high-pass + butterwroth low-pass
struct BandPass {
    Biquad hp, lp;

    void setup(float lowHz, float highHz, float fs) {
        hp.setup(true, lowHz, fs);
        lp.setup(false, highHz, fs);
    }

    // Call with the first raw sample when starting (or restarting after motion).
    void reset(float firstSample) {
        hp.reset(firstSample);
        lp.reset(0.0f); // high-pass output starts at 0
    }

    float process(float x) {
        return lp.process(hp.process(x));
    }
};
