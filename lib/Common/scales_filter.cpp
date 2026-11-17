#include "scales_filter.h"
#include "drip_tray_guard.h"

#include <algorithm>
#include <cmath>

namespace {
// Three robust standard deviations, with a 1 g floor for ordinary ADC noise.
constexpr float MIN_DEVIATION_G = 1.f;
constexpr float MAD_MULTIPLIER = 4.5f;
// A sustained new load may move while it is being placed or water is arriving.
constexpr float MAX_RATE_G_PER_S = 20.f;

float median(float *values, unsigned count) {
  std::sort(values, values + count);
  return count % 2 ? values[count / 2]
    : (values[count / 2 - 1] + values[count / 2]) * 0.5f;
}
}

void ScalesFilter::reset() {
  count = head = pendingCount = 0;
  haveInput = false;
}

void ScalesFilter::remember(const float value[2]) {
  history[head][0] = value[0];
  history[head][1] = value[1];
  head = (head + 1) % HISTORY_SIZE;
  if (count < HISTORY_SIZE) ++count;
}

bool ScalesFilter::matchesHistory(const float value[2]) const {
  if (!count) return true;
  for (unsigned channel = 0; channel < 2; ++channel) {
    float values[HISTORY_SIZE], deviations[HISTORY_SIZE];
    for (unsigned i = 0; i < count; ++i) values[i] = history[i][channel];
    const float center = median(values, count);
    for (unsigned i = 0; i < count; ++i) deviations[i] = std::fabs(values[i] - center);
    const float adaptive = MAD_MULTIPLIER * median(deviations, count);
    const float limit = adaptive > MIN_DEVIATION_G ? adaptive : MIN_DEVIATION_G;
    if (std::fabs(value[channel] - center) > limit) return false;
  }
  return true;
}

bool ScalesFilter::matchesPrevious(const float value[2], uint32_t elapsed) const {
  if (!count) return true;
  const float rateLimit = MAX_RATE_G_PER_S * elapsed / 1000.f;
  const float limit = rateLimit > MIN_DEVIATION_G ? rateLimit : MIN_DEVIATION_G;
  const unsigned previous = (head + HISTORY_SIZE - 1) % HISTORY_SIZE;
  return std::fabs(value[0] - history[previous][0]) <= limit &&
         std::fabs(value[1] - history[previous][1]) <= limit;
}

ScaleSampleResult ScalesFilter::update(const long raw[2], const float factors[2], uint32_t now) {
  // A real conversion outage starts a new history; a run of rejected conversions
  // does not refresh or erase the old accepted reference.
  if (haveInput && !dripTraySampleFresh(true, lastInput, now)) reset();
  const uint32_t elapsed = now - lastInput;
  lastInput = now;
  haveInput = true;

  if (!dripTrayRawPairValid(raw) || !std::isfinite(factors[0]) || !std::isfinite(factors[1]) ||
      factors[0] == 0.f || factors[1] == 0.f) {
    pendingCount = 0;
    return ScaleSampleResult::Invalid;
  }
  const float value[2] = {raw[0] / factors[0], raw[1] / factors[1]};
  if (!std::isfinite(value[0]) || !std::isfinite(value[1])) {
    pendingCount = 0;
    return ScaleSampleResult::Invalid;
  }
  // Permit a normal flow ramp to begin from a quiet baseline without waiting
  // for the median to catch up. Once a jump is pending it must be confirmed.
  if (matchesHistory(value) || (!pendingCount && matchesPrevious(value, elapsed))) {
    pendingCount = 0;
    remember(value);
    return ScaleSampleResult::Accepted;
  }

  const float rateLimit = MAX_RATE_G_PER_S * elapsed / 1000.f;
  const float confirmLimit = rateLimit > MIN_DEVIATION_G ? rateLimit : MIN_DEVIATION_G;
  if (pendingCount && (std::fabs(value[0] - pending[pendingCount - 1][0]) > confirmLimit ||
                      std::fabs(value[1] - pending[pendingCount - 1][1]) > confirmLimit))
    pendingCount = 0;
  pending[pendingCount][0] = value[0];
  pending[pendingCount][1] = value[1];
  if (++pendingCount < CONFIRM_SAMPLES) return ScaleSampleResult::Outlier;

  // Rebase only after three new, mutually consistent conversions. The caller
  // must still rebuild its safety window using subsequent accepted samples.
  count = head = 0;
  for (unsigned i = 0; i < pendingCount; ++i) remember(pending[i]);
  pendingCount = 0;
  return ScaleSampleResult::LoadChanged;
}
