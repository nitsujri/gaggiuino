#include "shot_stop.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float MAX_FLOW_G_PER_S = 20.f;
}

bool ShotStopPrediction::shouldStop(float targetG) const {
  if (!active || !std::isfinite(targetG) || targetG <= 0.f) return false;
  // Weight-controlled brewing must not silently become pump-based estimation
  // after the limited bridge through rejected/missing scale conversions.
  return expired || (std::isfinite(measuredWeightG) && measuredWeightG >= targetG) ||
                   (std::isfinite(finalWeightG) && finalWeightG >= targetG);
}

void ShotStopPredictor::reset(uint32_t now) {
  count = head = 0;
  active = true;
  startedAt = now;
  expectedFlow = 0.f;
}

bool ShotStopPredictor::add(const ScaleControlSample &sample, uint32_t now) {
  if (!active || !sample.valid || !std::isfinite(sample.weightG) ||
      now - sample.sampledMillis > MAX_SAMPLE_AGE_MS ||
      sample.smoothingDelayMs > MAX_SAMPLE_AGE_MS) return false;
  if (count) {
    const auto &previous = samples[(head + HISTORY_SIZE - 1) % HISTORY_SIZE];
    const uint32_t elapsed = sample.sampledMillis - previous.sampledMillis;
    if (!elapsed || elapsed > INT32_MAX) return false; // duplicate or out of order
    if (elapsed > MAX_SAMPLE_AGE_MS) count = head = 0;
  }
  samples[head] = sample;
  head = (head + 1) % HISTORY_SIZE;
  if (count < HISTORY_SIZE) ++count;
  expectedFlow = estimateFlow();
  return true;
}

float ShotStopPredictor::estimateFlow() const {
  if (count < HISTORY_SIZE) return 0.f;
  float rates[HISTORY_SIZE - 1];
  for (unsigned i = 1; i < HISTORY_SIZE; ++i) {
    const auto &before = samples[(head + i - 1) % HISTORY_SIZE];
    const auto &after = samples[(head + i) % HISTORY_SIZE];
    const uint32_t elapsed = (after.sampledMillis - after.smoothingDelayMs)
      - (before.sampledMillis - before.smoothingDelayMs);
    if (!elapsed || elapsed > MAX_SAMPLE_AGE_MS * 2) return 0.f;
    rates[i - 1] = (after.weightG - before.weightG) * 1000.f / elapsed;
    if (!std::isfinite(rates[i - 1])) return 0.f;
  }
  // A surviving mass spike produces a positive then negative rate; the median
  // of three signed intervals rejects that pair without resetting flow on a
  // cached reading. Recompute only on new accepted observations.
  std::sort(rates, rates + (HISTORY_SIZE - 1));
  const float estimate = rates[1];
  if (estimate < 0.f) return 0.f;
  return estimate > MAX_FLOW_G_PER_S ? MAX_FLOW_G_PER_S : estimate;
}

ShotStopPrediction ShotStopPredictor::predict(uint32_t now, uint32_t tailMs) const {
  ShotStopPrediction prediction;
  prediction.active = active;
  if (!active) return prediction;
  if (!count) {
    prediction.expired = now - startedAt > MAX_SAMPLE_AGE_MS;
    return prediction;
  }
  const auto &latest = samples[(head + HISTORY_SIZE - 1) % HISTORY_SIZE];
  prediction.sampleAgeMs = now - latest.sampledMillis;
  prediction.smoothingDelayMs = latest.smoothingDelayMs;
  prediction.expired = prediction.sampleAgeMs > MAX_SAMPLE_AGE_MS;
  prediction.measuredWeightG = latest.weightG;
  prediction.currentWeightG = prediction.finalWeightG = latest.weightG;
  if (prediction.expired) return prediction;
  prediction.flowReady = count == HISTORY_SIZE;
  prediction.flowGPerS = expectedFlow;
  const uint32_t boundedTail = tailMs > 2000 ? 2000 : tailMs;
  const float measurementAge = (prediction.sampleAgeMs + prediction.smoothingDelayMs) / 1000.f;
  prediction.currentWeightG += prediction.flowGPerS * measurementAge;
  prediction.finalWeightG = prediction.currentWeightG + prediction.flowGPerS * (boundedTail / 1000.f);
  return prediction;
}
