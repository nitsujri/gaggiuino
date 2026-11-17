#ifndef SCALES_FILTER_H
#define SCALES_FILTER_H

#include <stdint.h>

enum class ScaleSampleResult { Accepted, Outlier, Invalid, LoadChanged };

// Call once per new local ADC conversion, before display smoothing or safety
// settling. Rejected samples never enter the accepted history.
class ScalesFilter {
public:
  void reset();
  ScaleSampleResult update(const long raw[2], const float factors[2], uint32_t now);

private:
  static constexpr unsigned HISTORY_SIZE = 5;
  static constexpr unsigned CONFIRM_SAMPLES = 3;
  float history[HISTORY_SIZE][2] = {};
  float pending[CONFIRM_SAMPLES][2] = {};
  unsigned count = 0, head = 0, pendingCount = 0;
  bool haveInput = false;
  uint32_t lastInput = 0;

  void remember(const float value[2]);
  bool matchesHistory(const float value[2]) const;
  bool matchesPrevious(const float value[2], uint32_t elapsed) const;
};

#endif
