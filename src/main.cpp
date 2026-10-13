#include <Wire.h>
#include <Adafruit_LIS3DH.h>

// Step counter based on this algorithm for ADXL367: https://www.analog.com/en/resources/app-notes/an-2554.html

#define COUNTS_PER_G    16384.0f // LIS3DH gives 16-bit signed accelerations. We configure it for ±2 g. Then 16384 = 1g
#define SAMPLE_HZ       50
#define FILTER_ORDER    4 // number of samples in the moving average filter
#define THRESH_ORDER    4 // number of recent midpoints to use for the dynamic threshold
#define WINDOW             (FILTER_ORDER * 4 + 1)  // number of samples to search for a peak. 17 samples = 0.34 s
#define HALF            (WINDOW / 2)                
#define SENS            0.1f                    // sensitivity in g. Count a step when max and min above and below the threshold and amplitude is higher than this
#define MIN_SEARCH      SAMPLE_HZ               // 1 s to find the min after a max
#define REG_OFF_TIME    (2 * SAMPLE_HZ)         // number of samples without a step to leave walking mode
#define STEPS_TO_START  8                       // number of steps before we start counting
#define MAX_FAILS       2                       // consecutive failed pairs before reset

Adafruit_LIS3DH lis;

// circular buffers
// last (FILTER_ORDER) raw values
float filt[FILTER_ORDER];
uint8_t fIdx = 0;
// last (WINDOW) filtered values to search for a local max and min within
float win[WINDOW];
uint8_t wIdx = 0;

// last (THRESH_ORDER) midpoints and their average to use as a threshold
// we want a moving threshold because the resting value might vary instead of remaining exactly 1g
// so when max - min is significant, we will update the estimate of the center
float thrBuf[THRESH_ORDER];
uint8_t tIdx = 0;
float dynThr = 0;

// samples processed
uint32_t n = 0;

enum { FIND_MAX, FIND_MIN } state = FIND_MAX;
float maxVal = 0;
uint8_t minWait = 0;
uint8_t sinceStep = 0;
uint8_t fails = 0;

// num valid pairs before we reach STEPS_TO_START and confirm walking
int possible = 0;
bool isWalking = false;

// total steps
uint32_t steps = 0;

void setup() {
  Serial.begin(115200);
  uint32_t t = millis();
  while (!Serial && millis() - t < 2000) delay(10); // wait but don't block forever
  if (!lis.begin(0x18)) { Serial.println("No LIS3DH"); while (1); }
  lis.setRange(LIS3DH_RANGE_2_G);
  lis.setDataRate(LIS3DH_DATARATE_50_HZ);
}

// check if step and update threshold
void evalPair(float mx, float mn) {
  bool isStep = mx > dynThr + SENS / 2 && mn < dynThr - SENS / 2;

  // update the threshold on big swings in acceleration
  // use SENS so that a step always affects the threshold
  if (mx - mn > SENS) {
    thrBuf[tIdx] = (mx + mn) / 2;
    tIdx = (tIdx + 1) % THRESH_ORDER;
    dynThr = 0;
    for (float v : thrBuf) {
      dynThr += v;
    }
    dynThr /= THRESH_ORDER;
  }

  if (isStep) {
    fails = 0;
    sinceStep = 0;
    if (isWalking) steps++;
    else if (++possible >= STEPS_TO_START) {
      steps += possible;
      possible = 0;
      isWalking = true;
    }
    Serial.print("steps: ");
    Serial.println(steps);
  } else if (++fails >= MAX_FAILS) {
    fails = 0;
    possible = 0;
    isWalking = false;
  }
}

// handle a sampled acceleration
void process(float a) {
  // add to the filter buffer and compute moving average to use instead, smoothing out spikes
  filt[fIdx] = a;
  fIdx = (fIdx + 1) % FILTER_ORDER;

  float s = 0;
  for (float v : filt) {
    s += v;
  }
  s /= FILTER_ORDER;

  // add to the window of filtered samples
  win[wIdx] = s;
  wIdx = (wIdx + 1) % WINDOW;
  n++;

  if (n < WINDOW + FILTER_ORDER) {
    // why are we doing this?
    for (float &v : thrBuf) {
      v = s;
    }
    dynThr = s;
    return;
  }

  // 3. Center-of-window peak test. Strict vs. older samples, non-strict vs.
  //    newer, so a plateau yields exactly one peak and a flat signal yields none.

  uint8_t center = (wIdx + HALF) % WINDOW;
  float v = win[center];
  bool isMax = true, isMin = true;

  // is the center a local max or min?
  for (uint8_t k = 1; k <= HALF; k++) {
    float older = win[(center + WINDOW - k) % WINDOW];
    float newer = win[(center + k) % WINDOW];
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
  } else if (++minWait >= MIN_SEARCH) {
    // min not found. reset possible steps, if in walking mode stay in walking mode
    state = FIND_MAX;
    possible = 0; 
  }

  // leave walking mode after 2 s without a valid step
  if (++sinceStep >= REG_OFF_TIME) {
    sinceStep = 0;
    possible = 0;
    isWalking = false;
  }
}

void loop() {
  // TODO: switch this to interrupts, move step counting out of main
  if (!lis.haveNewData()) {
    return;
  }
  lis.read();
  float a = sqrtf((float)lis.x * lis.x + (float)lis.y * lis.y + (float)lis.z * lis.z) / COUNTS_PER_G;
  process(a);
}