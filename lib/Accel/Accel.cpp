#include <Wire.h>
#include <Adafruit_LIS3DH.h>
#include <Accel.h>
#include <Debug.h>

// Step counter based on this algorithm for ADXL367: https://www.analog.com/en/resources/app-notes/an-2554.html
Adafruit_LIS3DH lis;
bool foundLIS3DH = false;

// circular buffers
// last (ACCEL_FILTER_ORDER) raw values
float filt[ACCEL_FILTER_ORDER];
uint8_t fIdx = 0;
// last (ACCEL_WINDOW) filtered values to search for a local max and min within
float win[ACCEL_WINDOW];
uint8_t wIdx = 0;

// last (ACCEL_THRESH_ORDER) midpoints and their average to use as a threshold
// we want a moving threshold because the resting value might vary instead of remaining exactly 1g
// so when max - min is significant, we will update the estimate of the center
float thrBuf[ACCEL_THRESH_ORDER];
uint8_t tIdx = 0;
float dynThr = 0;

// samples processed
uint32_t n = 0;

enum State state = FIND_MAX;
float maxVal = 0;
uint8_t minWait = 0;
uint8_t sinceStep = 0;
uint8_t fails = 0;

// num valid pairs before we reach ACCEL_STEPS_TO_START and confirm walking
int possible = 0;
bool isWalking = false;

// total steps
uint32_t steps = 0;

// check if step and update threshold
void evalPair(float mx, float mn) {
  bool isStep = mx > dynThr + ACCEL_SENS / 2 && mn < dynThr - ACCEL_SENS / 2;

  // update the threshold on big swings in acceleration
  // use ACCEL_SENS so that a step always affects the threshold
  if (mx - mn > ACCEL_SENS) {
    thrBuf[tIdx] = (mx + mn) / 2;
    tIdx = (tIdx + 1) % ACCEL_THRESH_ORDER;
    dynThr = 0;
    for (float v : thrBuf) {
      dynThr += v;
    }
    dynThr /= ACCEL_THRESH_ORDER;
  }

  if (isStep) {
    fails = 0;
    sinceStep = 0;
    if (isWalking) steps++;
    else if (++possible >= ACCEL_STEPS_TO_START) {
      steps += possible;
      possible = 0;
      isWalking = true;
    }
    DEBUG_PRINT("steps: ");
    DEBUG_PRINTLN(steps);
  } else if (++fails >= ACCEL_MAX_FAILS) {
    fails = 0;
    possible = 0;
    isWalking = false;
  }
}

// handle a sampled acceleration
void process(float a) {
  // add to the filter buffer and compute moving average to use instead, smoothing out spikes
  filt[fIdx] = a;
  fIdx = (fIdx + 1) % ACCEL_FILTER_ORDER;

  float s = 0;
  for (float v : filt) {
    s += v;
  }
  s /= ACCEL_FILTER_ORDER;

  // add to the window of filtered samples
  win[wIdx] = s;
  wIdx = (wIdx + 1) % ACCEL_WINDOW;
  n++;

  if (n < ACCEL_WINDOW + ACCEL_FILTER_ORDER) {
    for (float &v : thrBuf) {
      v = s;
    }
    dynThr = s;
    return;
  }

  uint8_t center = (wIdx + ACCEL_HALF) % ACCEL_WINDOW;
  float v = win[center];
  bool isMax = true, isMin = true;

  // is the center a local max or min?
  for (uint8_t k = 1; k <= ACCEL_HALF; k++) {
    float older = win[(center + ACCEL_WINDOW - k) % ACCEL_WINDOW];
    float newer = win[(center + k) % ACCEL_WINDOW];
    // check older using = in case of flat top, e.g.:
    // ... 1.30  1.42  1.50  1.50  1.50  1.41  1.28 ...
    // max at 1.50 only counts once(first time)
    if (older >= v || newer > v) isMax = false;
    if (older <= v || newer < v) isMin = false;
  }

  if (state == FIND_MAX) {
    if (isMax) { maxVal = v; minWait = 0; state = FIND_MIN; }
  } else if (isMin) {
    evalPair(maxVal, v);
    state = FIND_MAX;
  } else if (++minWait >= ACCEL_MIN_SEARCH) {
    // min not found. reset possible steps, if in walking mode stay in walking mode
    state = FIND_MAX;
    possible = 0; 
  }

  // leave walking mode after 2 s without a valid step
  if (++sinceStep >= ACCEL_REG_OFF_TIME) {
    sinceStep = 0;
    possible = 0;
    isWalking = false;
  }
}

// call Wire.begin() and Serial.begin() first
bool setupAccelFIFO(void) {
  uint8_t err;
  if (!lis.begin(0x18)) {
    DEBUG_PRINTLN("No LIS3DH");
    return false;
  }
  foundLIS3DH = true;
  lis.setRange(LIS3DH_RANGE_2_G);
  lis.setDataRate(LIS3DH_DATARATE_50_HZ);

  Wire.beginTransmission(0x18);
  Wire.write(LIS3DH_REG_CTRL5);
  Wire.write(0b01000000); // set FIFO_EN

  err = Wire.endTransmission();
  if (err) goto onerr;
  
  /*
    LIS3DH FIFO modes
    Bypass mode (00). The FIFO is not used.
    FIFO mode (01). The buffer fills with data until 32 samples are stored, then it stops collecting data. FIFO mode must be reset later.
    Stream mode (10). The buffer fills with new samples continuously. When it is full it begins overwriting older data.
    Stream-to-FIFO mode (11). The buffer starts in Stream mode and switches to FIFO when an interrupt occurs.
  */

  Wire.beginTransmission(0x18);
  Wire.write(LIS3DH_REG_FIFOCTRL);
  Wire.write(0b10011000); // Use FIFO stream mode; read up to 24 entries at once

  err = Wire.endTransmission();
  if (err) goto onerr;

  Wire.beginTransmission(0x18);
  Wire.write(LIS3DH_REG_CTRL3);
  Wire.write(0b00000100); // Enable watermark interrupt

  err = Wire.endTransmission();
  if (err) goto onerr;

  return true;

  onerr:
    DEBUG_PRINT("I2C error(LIS3DH): ");
    DEBUG_PRINTLN(err);
    foundLIS3DH = false;
    return false;
}

void drainFIFO(void) {
  
  if (!foundLIS3DH) {
    return;
  }
  Wire.beginTransmission(0x18);
  Wire.write(LIS3DH_REG_FIFOSRC);
  uint8_t err = Wire.endTransmission(false);
  if (err) {
    DEBUG_PRINT("I2C error (drainFIFO): ");
    DEBUG_PRINTLN(err);
    return;
  } 
  uint8_t numToRead;
  if (Wire.requestFrom(0x18, 1) == 1) {
    numToRead = Wire.read();
    numToRead &= ((1<<5) - 1);
  } else {
    DEBUG_PRINTLN("couldn't read reg");
    return;
  }

  while (numToRead > 0) {
    lis.read();
    float a = sqrtf((float)lis.x * lis.x + (float)lis.y * lis.y + (float)lis.z * lis.z) / ACCEL_COUNTS_PER_G;
    process(a); 
    numToRead--;
  }
}