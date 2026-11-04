#ifndef SCALES_H
#define SCALES_H

#ifndef FORCE_PREDICTIVE_SCALES
#define FORCE_PREDICTIVE_SCALES 0
#endif
#ifndef DRIP_TRAY_GUARD_ENABLED
#define DRIP_TRAY_GUARD_ENABLED 0
#endif
#ifndef DRIP_TRAY_MIN_WEIGHT_G
#define DRIP_TRAY_MIN_WEIGHT_G 150.f
#endif
#ifndef DRIP_TRAY_RETRY_MS
#define DRIP_TRAY_RETRY_MS 1000UL
#endif

#include "measurements.h"
#include "drip_tray_guard.h"

// Read-only SWD service snapshot. Raw values are absolute and independent of tare.
struct ScalesDiagnostics {
  int32_t meanRaw[2];
  float spreadG;
  uint32_t samples;
  uint32_t sampleMillis;
  uint32_t now;
  float grossWeight;
  uint32_t trayStatus;
};
extern volatile ScalesDiagnostics scalesDiagnostics;

void scalesInit(float scalesF1, float scalesF2);
void scalesPoll(void);
void scalesTare(void);
Measurement scalesGetWeight(void);
// Settled local mean, independent of display tare and remote/predictive scales.
// Returns NaN when local conversions are unavailable, stale or unsettled.
Measurement scalesGetSettledAbsoluteWeight(void);
bool scalesIsPresent(void);
DripTrayStatus scalesGetDripTrayStatus(float *grossWeight = nullptr);

#endif
