#ifndef BOILER_FILL_H
#define BOILER_FILL_H

#include "drip_tray_guard.h"

enum class BoilerFillState { Waiting, Filling, Complete, TimedOut };

// Uses local absolute scale readings, so display tare cannot look like water.
class BoilerFill {
public:
  static constexpr uint32_t START_DELAY_MS = 3000;
  static constexpr uint32_t BASELINE_SETTLE_MS = 1000;
  static constexpr float BASELINE_SPREAD_G = 0.5f;
  static constexpr float WATER_WEIGHT_G = 3.f;
  static constexpr uint32_t WATER_CONFIRM_MS = 1000;
  static constexpr uint32_t MIN_PUMP_MS = 2000;
  static constexpr uint32_t MAX_PUMP_MS = 30000;

  void reset();
  BoilerFillState update(float absoluteWeight, uint32_t sampleTime, uint32_t now);

private:
  BoilerFillState state = BoilerFillState::Waiting;
  BoilerFillClock clock;
  bool haveSample = false, confirmingWater = false;
  uint32_t lastSample = 0, baselineSince = 0, pumpSince = 0, waterSince = 0;
  float baselineLow = 0.f, baselineHigh = 0.f, baseline = 0.f;
};

#endif
