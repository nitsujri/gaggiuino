#include "scales.h"
#include "pindef.h"
#include "remote_scales.h"
#include "scales_clock.h"
#include "../log.h"
#include <HX711_2.h>
#include <algorithm>
#include <cmath>

volatile ScalesDiagnostics scalesDiagnostics = {};
namespace {
HX711_2 &cells() {
  static HX711_2 value(TIM3);
  return value;
}
bool hardwarePresent = false, haveSample = false;
long absoluteRaw[2] = {}, recent[4][2] = {};
unsigned count = 0, head = 0;
uint32_t lastSample = 0;
uint32_t initializedAt = 0;
float factors[2] = {};
bool fresh() { return dripTraySampleFresh(haveSample, lastSample, millis()); }
}

void scalesPoll() {
  scalesDiagnostics.now = millis();
  if (!hardwarePresent) return;
  long raw[2], offset[2];
  if (!cells().try_read(raw)) return;
  cells().get_offset(offset);
  const long next[2] = {raw[0] + offset[0], raw[1] + offset[1]};
  if (!dripTrayRawPairValid(next)) {
    haveSample = false;
    count = 0;
    scalesDiagnostics.samples = 0;
    return;
  }
  if (!fresh()) count = head = 0;
  for (unsigned j = 0; j < 2; ++j) {
    absoluteRaw[j] = next[j];
    recent[head][j] = next[j];
  }
  head = (head + 1) % 4;
  count = std::min(count + 1, 4U);
  haveSample = true;
  lastSample = millis();
  float spread = 0;
  for (unsigned j = 0; j < 2; ++j) {
    int64_t sum = 0;
    long lo = recent[0][j], hi = lo;
    for (unsigned i = 0; i < count; ++i) {
      sum += recent[i][j];
      lo = std::min(lo, recent[i][j]);
      hi = std::max(hi, recent[i][j]);
    }
    scalesDiagnostics.meanRaw[j] = sum / count;
    if (!std::isfinite(factors[j]) || factors[j] == 0) spread = INFINITY;
    else spread += std::fabs((hi - lo) / factors[j]);
  }
  scalesDiagnostics.spreadG = spread;
  scalesDiagnostics.samples = count;
  scalesDiagnostics.sampleMillis = lastSample;
  float gross;
  scalesDiagnostics.trayStatus = static_cast<uint32_t>(scalesGetDripTrayStatus(&gross));
  scalesDiagnostics.grossWeight = gross;
}

void scalesInit(float f1, float f2) {
  hardwarePresent = haveSample = false;
  count = head = 0;
  if (FORCE_PREDICTIVE_SCALES) return;
#ifndef DISABLE_HW_SCALES
  auto &c = cells();
#ifdef SINGLE_HX711_BOARD
  const auto clockMode = OUTPUT;
#else
  const auto clockMode = OUTPUT_OPEN_DRAIN;
#endif
  c.begin(HX711_dout_1, HX711_dout_2, HX711_sck_1, 128U, clockMode);
  configureScalesClock(HX711_sck_1, TIM3);
  factors[0] = f1;
  factors[1] = f2;
  c.set_scale(f1, f2);
  c.power_up();
  if (c.wait_ready_timeout(1000, 10)) {
    c.tare(4);
    hardwarePresent = true;
    LOG_INFO("HX711 detected; tare complete; waiting for settled samples");
  } else {
    c.power_down();
    LOG_ERROR("HX711 initialization timed out after 1000 ms");
  }
#endif
  initializedAt = millis();
  if (!hardwarePresent && remoteScalesIsPresent()) remoteScalesTare();
}

void scalesTare() {
  if (hardwarePresent && fresh() && count) {
    int64_t sum[2] = {};
    for (unsigned i = 0; i < count; ++i)
      for (unsigned j = 0; j < 2; ++j) sum[j] += recent[i][j];
    cells().set_offset(sum[0] / count, sum[1] / count);
  } else if (!hardwarePresent && remoteScalesIsPresent()) remoteScalesTare();
}

Measurement scalesGetWeight() {
  if (hardwarePresent && fresh()) {
    long offset[2];
    cells().get_offset(offset);
    const long zeros[2] = {};
    const float weight = calculateDripTrayGrossWeight(absoluteRaw, zeros, offset, factors);
    if (std::isfinite(weight)) return {weight, lastSample};
  }
  if (remoteScalesIsPresent()) return remoteScalesGetWeight();
  return {0.f, 0};
}

bool scalesIsPresent() {
  return !FORCE_PREDICTIVE_SCALES &&
         ((hardwarePresent && fresh()) || remoteScalesIsPresent());
}

Measurement scalesGetSettledAbsoluteWeight() {
  if (hardwarePresent && fresh() && dripTrayReadingsSettled(count, scalesDiagnostics.spreadG)) {
    const long mean[2] = {scalesDiagnostics.meanRaw[0], scalesDiagnostics.meanRaw[1]};
    const long zeros[2] = {};
    return {calculateDripTrayGrossWeight(mean, zeros, zeros, factors), lastSample};
  }
  return {NAN, 0};
}

DripTrayStatus scalesGetDripTrayStatus(float *gross) {
  if (gross) *gross = NAN;
#if defined(DRIP_TRAY_ZERO_RAW_1) && defined(DRIP_TRAY_ZERO_RAW_2)
  const bool ready = hardwarePresent && fresh() &&
                     dripTrayReadingsSettled(count, scalesDiagnostics.spreadG);
  if (dripTrayStartupPending(hardwarePresent, ready, initializedAt, millis()))
    return DripTrayStatus::Settling;
  if (!ready) return DripTrayStatus::Unavailable;
  const long baseline[2] = {DRIP_TRAY_ZERO_RAW_1, DRIP_TRAY_ZERO_RAW_2};
  const long zeroOffset[2] = {};
  const float weight = calculateDripTrayGrossWeight(absoluteRaw, zeroOffset, baseline, factors);
  if (gross) *gross = weight;
  return classifyDripTray(true, hardwarePresent, std::isfinite(weight), weight, DRIP_TRAY_MIN_WEIGHT_G);
#else
  return DripTrayStatus::Unconfigured;
#endif
}
