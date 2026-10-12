#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <Adafruit_LIS3DH.h>
#include "MAX30105.h"
#include "heartRate.h"


Adafruit_LIS3DH lis;

int steps = 0;

void setup(void) {
  Serial.begin(115200);
  while (!Serial) delay(10);

  lis = Adafruit_LIS3DH();
  if (!lis.begin(0x18)) {
    Serial.println("Could not start LIS3DH");
    while (1);
  } else {
    Serial.println("LIS3DH found!");
  }
  lis.setRange(LIS3DH_RANGE_2_G);
}

void loop() {
  lis.read();
  Serial.print("X: ");
  Serial.print(lis.x);
  Serial.print("\tY: ");
  Serial.print(lis.y);
  Serial.print("\tZ: ");
  Serial.println(lis.z);
  delay(20);
}