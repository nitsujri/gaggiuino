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
#include "shot_stop.h"

// Read-only SWD service snapshot. Raw values are absolute and independent of tare.
struct ScalesDiagnostics {
  int32_t meanRaw[2];
  float spreadG;
  uint32_t samples;
  uint32_t sampleMillis;
  uint32_t now;
  float grossWeight;
  uint32_t trayStatus;
  // Keep the original 32-byte prefix above stable for existing SWD captures.
  int32_t observedRaw[2];
  uint32_t observedMillis;
  uint32_t rejectedSamples;
  uint32_t confirmedLoadChanges;
  uint32_t filterStatus; // ScaleSampleResult
  int32_t displayedRaw[2];
};
static_assert(sizeof(ScalesDiagnostics) == 64, "Update the SWD reader when changing the scale snapshot");
extern volatile ScalesDiagnostics scalesDiagnostics;

void scalesInit(float scalesF1, float scalesF2);
void scalesPoll(void);
bool scalesTare(void);
Measurement scalesGetWeight(void);
// Retains accepted timestamps during rejection; the shot predictor enforces age.
ScaleControlSample scalesGetControlWeight(void);
bool scalesHasLocalHardware(void);
// Settled local mean, independent of display tare and remote/predictive scales.
// Returns NaN when local conversions are unavailable, stale or unsettled.
Measurement scalesGetSettledAbsoluteWeight(void);
bool scalesIsPresent(void);
DripTrayStatus scalesGetDripTrayStatus(float *grossWeight = nullptr);

#endif
