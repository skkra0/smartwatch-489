#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <Adafruit_LIS3DH.h>
#include "MAX30105.h"
#include "heartRate.h"

// LIS3DH stores 16-bit acceleration values with range of ±2g: divide by 2^14 to get gs
#define COUNTS_PER_G (16384.0f)


Adafruit_LIS3DH lis;
float stepThreshold = 1.3;
unsigned long debounceTime = 300; // ms
unsigned long lastStepTime = 0;

int steps = 0;

void setup(void) {
  Serial.begin(115200);

  lis = Adafruit_LIS3DH();
  if (!lis.begin(0x18)) {
    Serial.println("Could not start LIS3DH");
    while (1);
  } else {
    Serial.println("LIS3DH found!");
  }
  lis.setRange(LIS3DH_RANGE_2_G);
  lis.setDataRate(LIS3DH_DATARATE_50_HZ);
}

void loop() {
  if (!lis.haveNewData()) {
    return;
  }

  lis.read();
  float norm = sqrtf(sq((float) lis.x) + sq((float) lis.y) + sq((float) lis.z)) / COUNTS_PER_G;
  unsigned long now = millis();
  if (norm > stepThreshold && (now - lastStepTime) > debounceTime) {
    steps++;
    lastStepTime = now;
    Serial.print("Total steps: ");
    Serial.println(steps);
  }
}