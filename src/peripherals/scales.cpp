#include "scales.h"
#include "indigo/control.h"
#include "pindef.h"
#include "remote_scales.h"
#include "scales_clock.h"
#include <HX711_2.h>
#include <algorithm>
#include <cmath>

namespace {
HX711_2 &cells() {
  static HX711_2 value(TIM3);
  return value;
}
bool hwScalesPresent = false;
eepromValues_t *trayCfg = nullptr;
long absoluteRaw[2] = {};
long recent[4][2] = {};
unsigned count = 0, head = 0;
uint32_t lastSample = 0;
bool haveSample = false;
float factors[2] = {3920.f, 4210.f};
bool fresh() { return sampleIsFresh(haveSample, lastSample, millis(), 300); }
} // namespace
void scalesConfigure(eepromValues_t &cfg) {
  trayCfg = &cfg;
  factors[0] = cfg.scalesF1;
  factors[1] = cfg.scalesF2;
  if (hwScalesPresent)
    cells().set_scale(factors[0], factors[1]);
}
void scalesPoll() {
  if (!hwScalesPresent)
    return;
  long raw[2], offset[2];
  if (!cells().try_read(raw))
    return;
  cells().get_offset(offset);
  const long next[2] = {raw[0] + offset[0], raw[1] + offset[1]};
  if (!hx711RawPairValid(next)) {
    haveSample = false;
    count = 0;
    head = 0;
    return;
  }
  if (!fresh()) {
    count = 0;
    head = 0;
  }
  for (unsigned i = 0; i < 2; i++) {
    absoluteRaw[i] = next[i];
    recent[head][i] = absoluteRaw[i];
  }
  head = (head + 1) % 4;
  if (count < 4)
    count++;
  lastSample = millis();
  haveSample = true;
}
void scalesInit(float f1, float f2) {
  hwScalesPresent = false;
  haveSample = false;
  count = 0;
  if (FORCE_PREDICTIVE_SCALES)
    return;
#ifndef DISABLE_HW_SCALES
  auto &c = cells();
#ifdef SINGLE_HX711_BOARD
  const auto clockMode = OUTPUT;
#else
  const auto clockMode = OUTPUT_OPEN_DRAIN;
#endif
  c.begin(HX711_dout_1, HX711_dout_2, HX711_sck_1, 128U, clockMode);
#if defined(ARDUINO_ARCH_STM32)
  configureScalesClock(HX711_sck_1, TIM3);
#endif
  factors[0] = f1;
  factors[1] = f2;
  c.set_scale(f1, f2);
  c.power_up();
  if (c.wait_ready_timeout(1000, 10)) {
    c.tare(4);
    hwScalesPresent = true;
  } else
    c.power_down();
#endif
  if (!hwScalesPresent && remoteScalesIsPresent())
    remoteScalesTare();
}
void scalesTare() {
  if (hwScalesPresent && fresh() && count) {
    long mean[2] = {};
    for (unsigned i = 0; i < count; i++)
      for (unsigned j = 0; j < 2; j++)
        mean[j] += recent[i][j] / static_cast<long>(count);
    cells().set_offset(mean[0], mean[1]);
  } else if (remoteScalesIsPresent())
    remoteScalesTare();
}
Measurement scalesGetWeight() {
  if (hwScalesPresent && fresh()) {
    long offset[2];
    cells().get_offset(offset);
    if (factors[0] == 0 || factors[1] == 0)
      return {0.f, 0};
    const float w = (absoluteRaw[0] - offset[0]) / factors[0] +
                    (absoluteRaw[1] - offset[1]) / factors[1];
    if (std::isfinite(w))
      return {w, lastSample};
  }
  if (remoteScalesIsPresent())
    return remoteScalesGetWeight();
  return {0.f, 0};
}
bool scalesIsPresent() {
  return !FORCE_PREDICTIVE_SCALES &&
         ((hwScalesPresent && fresh()) || remoteScalesIsPresent());
}
DripTrayStatus scalesGetDripTrayStatus(float *gross) {
  if (gross)
    *gross = NAN;
  if (!trayCfg || !trayCfg->trayGuardCalibrated)
    return DripTrayStatus::Unconfigured;
  if (!hwScalesPresent || !fresh())
    return DripTrayStatus::Unavailable;
  const long zero[2] = {trayCfg->trayZeroRaw[0], trayCfg->trayZeroRaw[1]},
             noOffset[2] = {0, 0};
  const float w =
      calculateDripTrayGrossWeight(absoluteRaw, noOffset, zero, factors);
  if (gross)
    *gross = w;
  return classifyDripTray(true, true, std::isfinite(w), w,
                          trayCfg->trayMinWeightG);
}
bool scalesCaptureTrayZero(eepromValues_t &cfg) {
  if (!hwScalesPresent || !fresh() || count < 4)
    return false;
  int32_t zeros[2];
  for (unsigned j = 0; j < 2; j++) {
    long lo = recent[0][j], hi = lo;
    int64_t sum = 0;
    for (unsigned i = 0; i < count; i++) {
      lo = std::min(lo, recent[i][j]);
      hi = std::max(hi, recent[i][j]);
      sum += recent[i][j];
    }
    if (!std::isfinite(factors[j]) || factors[j] == 0 ||
        std::fabs((hi - lo) / factors[j]) > 1.f)
      return false;
    zeros[j] = static_cast<int32_t>(sum / count);
  }
  cfg.trayZeroRaw[0] = zeros[0];
  cfg.trayZeroRaw[1] = zeros[1];
  cfg.trayGuardCalibrated = true;
  return true;
}
