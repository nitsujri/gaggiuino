/* 09:32 15/03/2023 - change triggering comment */#ifndef SCALES_H
#define SCALES_H

// Needs to be here so it pleases so cppcheck is pleased.
#ifndef FORCE_PREDICTIVE_SCALES
#define FORCE_PREDICTIVE_SCALES 0
#endif

#ifndef DRIP_TRAY_GUARD_ENABLED
#define DRIP_TRAY_GUARD_ENABLED 0
#endif

#ifndef DRIP_TRAY_MIN_WEIGHT_G
#define DRIP_TRAY_MIN_WEIGHT_G 100.f
#endif

#ifndef DRIP_TRAY_RETRY_MS
#define DRIP_TRAY_RETRY_MS 1000UL
#endif

#include "measurements.h"
#include "drip_tray_guard.h"

void scalesInit(float scalesF1, float scalesF2);
void scalesTare(void);
Measurement scalesGetWeight(void);
bool scalesIsPresent(void);
DripTrayStatus scalesGetDripTrayStatus(float *grossWeight = nullptr);

#endif
