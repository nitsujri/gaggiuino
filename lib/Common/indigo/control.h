#pragma once
#include "drip_tray_guard.h"
#include "sensors_state.h"
#include <stdint.h>
inline bool sampleIsFresh(bool valid, uint32_t sampled, uint32_t now,
                          uint32_t age) {
  return valid && static_cast<uint32_t>(now - sampled) <= age;
}
// Explicit Home Flush is supervised by the user: no tray or startup
// prerequisite.
inline bool homeFlushAllowed(const SensorState &sensor, uint32_t displayAge) {
  return !sensor.brewSwitchState && !sensor.steamSwitchState &&
         !sensor.hotWaterSwitchState && sensor.waterLvl >= 10 &&
         displayAge < 1500;
}
// The fill budget starts only after its prerequisites are met. A missing tray
// must not use up the fill timeout and later enable heating without filling.
class StartupFillClock {
  bool started = false;
  uint32_t since = 0;

public:
  void reset() { started = false; }
  uint32_t elapsed(uint32_t now) {
    if (!started) {
      since = now;
      started = true;
    }
    return static_cast<uint32_t>(now - since);
  }
};

// Converter rails / hard-low data lines cannot establish a trustworthy load.
inline bool hx711RawPairValid(const long raw[2]) {
  for (unsigned i = 0; i < 2; i++)
    if (raw[i] == 0 || raw[i] <= -8388608L || raw[i] >= 8388607L)
      return false;
  return true;
}
