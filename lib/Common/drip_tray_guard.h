#ifndef DRIP_TRAY_GUARD_H
#define DRIP_TRAY_GUARD_H

#include <stdint.h>

enum class DripTrayStatus {
  Present,
  Missing,
  Unavailable,
  Unconfigured
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
