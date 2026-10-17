#include "drip_tray_guard.h"

#include <cmath>
#include <limits>

float calculateDripTrayGrossWeight(
  const long taredRaw[2],
  const long currentOffsets[2],
  const long noTrayOffsets[2],
  const float scaleFactors[2]
) {
  float grossWeight = 0.f;

  for (int channel = 0; channel < 2; channel++) {
    if (scaleFactors[channel] == 0.f || !std::isfinite(scaleFactors[channel])) {
      return std::numeric_limits<float>::quiet_NaN();
    }

    const int64_t grossRaw = static_cast<int64_t>(taredRaw[channel]) + currentOffsets[channel];
    const int64_t loadRaw = grossRaw - noTrayOffsets[channel];
    grossWeight += static_cast<float>(loadRaw) / scaleFactors[channel];
  }

  return grossWeight;
}

DripTrayStatus classifyDripTray(
  bool configured,
  bool hardwarePresent,
  bool readingValid,
  float grossWeight,
  float minimumWeight
) {
  if (!configured) {
    return DripTrayStatus::Unconfigured;
  }
  if (!std::isfinite(minimumWeight) || minimumWeight <= 0.f) {
    return DripTrayStatus::Unconfigured;
  }
  if (!hardwarePresent || !readingValid || !std::isfinite(grossWeight)) {
    return DripTrayStatus::Unavailable;
  }
  return grossWeight >= minimumWeight ? DripTrayStatus::Present : DripTrayStatus::Missing;
}
