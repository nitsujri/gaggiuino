#pragma once
#include "indigo/control.h"
#include "indigo/settings.h"
#include "sensors_state.h"
#include <Adafruit_VL53L0X.h>
#include <movingAvg.h>

class TOF {
  Adafruit_VL53L0X device;
  movingAvg average{4};
  uint16_t range = 0;
  uint32_t sampledAt = 0;
  bool ready = false, validSample = false;

public:
  void init(SensorState &sensor) {
#ifdef TOF_VL53L0X
    ready = device.begin(0x29, false, &Wire, Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_ACCURACY);
    if (ready) {
      average.begin();
      device.startRangeContinuous();
    }
#endif
    sensor.tofReady = ready;
  }
  void poll() {
#ifdef TOF_VL53L0X
    if (ready && device.isRangeComplete()) {
      const uint16_t mm = device.readRangeResult();
      // 8190/8191 are sensor out-of-range sentinels, not an empty tank calibration.
      if (device.readRangeStatus() == 0 && mm > 0 && mm < 8190) {
        range = average.reading(mm);
        sampledAt = millis();
        validSample = true;
      } else
        validSample = false;
    }
#endif
  }
  bool valid() const { return sampleIsFresh(validSample, sampledAt, millis(), 1000); }
  uint16_t distance() const { return range; }
  uint16_t readLvl(uint16_t full = 30) { return valid() ? tankPercent(range, full) : 9; }
  uint16_t readRangeToPct(uint16_t mm) { return tankPercent(mm, 30); }
};
