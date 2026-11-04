#include "boiler_fill.h"

#include <algorithm>
#include <cmath>

void BoilerFill::reset() {
  state = BoilerFillState::Waiting;
  clock.reset();
  haveSample = confirmingWater = false;
}

BoilerFillState BoilerFill::update(float weight, uint32_t sampleTime, uint32_t now) {
  if (state == BoilerFillState::Complete || state == BoilerFillState::TimedOut) return state;
  if (state == BoilerFillState::Filling && now - pumpSince >= MAX_PUMP_MS) {
    state = BoilerFillState::TimedOut;
    return state;
  }
  if (!std::isfinite(weight) || !dripTraySampleFresh(true, sampleTime, now)) {
    reset();
    return state;
  }

  const auto elapsed = clock.elapsed(now);
  // Re-reading a cached conversion must not advance settling or confirmation.
  if (haveSample && sampleTime == lastSample) return state;
  const bool firstSample = !haveSample;
  if (haveSample && !dripTraySampleFresh(true, lastSample, sampleTime)) {
    reset();
    return update(weight, sampleTime, now);
  }
  haveSample = true;
  lastSample = sampleTime;

  if (state == BoilerFillState::Waiting) {
    if (firstSample) {
      baselineLow = baselineHigh = weight;
      baselineSince = sampleTime;
    }
    baselineLow = std::min(baselineLow, weight);
    baselineHigh = std::max(baselineHigh, weight);
    if (baselineHigh - baselineLow > BASELINE_SPREAD_G) {
      baselineLow = baselineHigh = weight;
      baselineSince = sampleTime;
    }
    if (elapsed >= START_DELAY_MS && sampleTime - baselineSince >= BASELINE_SETTLE_MS) {
      // The high end of the settled baseline makes the water threshold conservative.
      baseline = baselineHigh;
      pumpSince = now;
      state = BoilerFillState::Filling;
    }
  } else if (weight - baseline >= WATER_WEIGHT_G) {
    if (!confirmingWater) {
      waterSince = sampleTime;
      confirmingWater = true;
    }
    if (sampleTime - waterSince >= WATER_CONFIRM_MS && now - pumpSince >= MIN_PUMP_MS)
      state = BoilerFillState::Complete;
  } else {
    confirmingWater = false;
  }
  return state;
}
