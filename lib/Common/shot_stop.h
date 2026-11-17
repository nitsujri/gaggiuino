#ifndef SHOT_STOP_H
#define SHOT_STOP_H

#include <stdint.h>

// Equivalent time for in-flight liquid, valve response and residual discharge.
// This is an initial machine-specific allowance, not a measured calibration.
#ifndef SHOT_STOP_TAIL_MS
#define SHOT_STOP_TAIL_MS 500
#endif
static_assert(SHOT_STOP_TAIL_MS >= 0 && SHOT_STOP_TAIL_MS <= 2000,
              "Shot stop tail must be between 0 and 2000 ms");

struct ScaleControlSample {
  float weightG = 0.f;
  uint32_t sampledMillis = 0;
  uint32_t smoothingDelayMs = 0;
  bool valid = false;
};

struct ShotStopPrediction {
  bool active = false;
  bool flowReady = false;
  bool expired = false;
  float measuredWeightG = 0.f;
  float currentWeightG = 0.f;
  float flowGPerS = 0.f;
  float finalWeightG = 0.f;
  uint32_t sampleAgeMs = 0;
  uint32_t smoothingDelayMs = 0;

  bool shouldStop(float targetG) const;
};

class ShotStopPredictor {
public:
  static constexpr uint32_t MAX_SAMPLE_AGE_MS = 500;
  void reset(uint32_t now);
  bool add(const ScaleControlSample &sample, uint32_t now);
  ShotStopPrediction predict(uint32_t now, uint32_t tailMs = SHOT_STOP_TAIL_MS) const;

private:
  static constexpr unsigned HISTORY_SIZE = 4; // three measured flow intervals
  ScaleControlSample samples[HISTORY_SIZE] = {};
  unsigned count = 0, head = 0;
  bool active = false;
  uint32_t startedAt = 0;
  float expectedFlow = 0.f;
  float estimateFlow() const;
};

#endif
