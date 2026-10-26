#include "drip_tray_guard.h"

#include <cmath>
#include <limits>

bool dripTraySampleFresh(bool haveSample, uint32_t sampleTime, uint32_t now) {
  return haveSample && static_cast<uint32_t>(now - sampleTime) <= 300;
}

bool dripTrayRawPairValid(const long raw[2]) {
  for (unsigned i = 0; i < 2; ++i)
    if (raw[i] == 0 || raw[i] <= -8388608L || raw[i] >= 8388607L) return false;
  return true;
}

bool dripTrayReadingsSettled(unsigned samples, float spreadG) {
  return samples >= 4 && std::isfinite(spreadG) && spreadG >= 0.f && spreadG <= 5.f;
}

bool dripTrayStartupPending(bool hardwarePresent, bool readingsReady,
                           uint32_t initializedAt, uint32_t now) {
  // This changes the notification, never the permission to discharge water.
  return hardwarePresent && !readingsReady &&
         static_cast<uint32_t>(now - initializedAt) < 3000U;
}

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
