#ifndef DRIP_TRAY_GUARD_H
#define DRIP_TRAY_GUARD_H

#include <stdint.h>

enum class DripTrayStatus {
  Present,
  Missing,
  Unavailable,
  Unconfigured,
  Settling
};

bool dripTraySampleFresh(bool haveSample, uint32_t sampleTime, uint32_t now);
bool dripTrayRawPairValid(const long raw[2]);
bool dripTrayReadingsSettled(unsigned samples, float spreadG);
bool dripTrayStartupPending(bool hardwarePresent, bool readingsReady,
                           uint32_t initializedAt, uint32_t now);

// Waiting for a tray must not consume the boiler-fill timeout.
class BoilerFillClock {
  bool started = false;
  uint32_t start = 0;
public:
  void reset() { started = false; }
  uint32_t elapsed(uint32_t now) {
    if (!started) { start = now; started = true; }
    return now - start;
  }
};

float calculateDripTrayGrossWeight(
  const long taredRaw[2],
  const long currentOffsets[2],
  const long noTrayOffsets[2],
  const float scaleFactors[2]
);

DripTrayStatus classifyDripTray(
  bool configured,
  bool hardwarePresent,
  bool readingValid,
  float grossWeight,
  float minimumWeight
);

#endif
