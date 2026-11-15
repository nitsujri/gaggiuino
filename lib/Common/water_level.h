#ifndef WATER_LEVEL_H
#define WATER_LEVEL_H

#include <stdint.h>

class WaterLevelAverage {
public:
  static constexpr uint32_t WINDOW_MS = 5000;
  static constexpr uint32_t SAMPLE_INTERVAL_MS = 50;

  void add(uint16_t distanceMm, uint32_t now) {
    expire(now);
    if (count && now - samples[(head + count - 1) % CAPACITY].time < SAMPLE_INTERVAL_MS)
      return;
    const uint16_t next = (head + count) % CAPACITY;
    samples[next] = {now, distanceMm};
    sum += distanceMm;
    ++count;
  }

  uint16_t reading(uint32_t now) {
    expire(now);
    return count ? (sum + count / 2u) / count : 0u;
  }

private:
  // Rate limiting bounds storage even when the main loop polls very quickly.
  static constexpr uint16_t CAPACITY = WINDOW_MS / SAMPLE_INTERVAL_MS + 1;
  struct Sample { uint32_t time; uint16_t distance; };
  Sample samples[CAPACITY] = {};
  uint32_t sum = 0;
  uint16_t head = 0, count = 0;

  void expire(uint32_t now) {
    while (count && now - samples[head].time >= WINDOW_MS) {
      sum -= samples[head].distance;
      head = (head + 1) % CAPACITY;
      --count;
    }
  }
};

// Distance grows as the tank empties. Keep the existing 125 mm safety boundary:
// 10% is usable, while 5% blocks operation via MIN_WATER_LVL (10%).
template <uint16_t FullDistanceMm>
uint16_t waterLevelPercent(uint16_t distanceMm) {
  static_assert(FullDistanceMm < 125u, "Full tank distance must be below 125 mm");
  if (distanceMm <= FullDistanceMm) return 100u;
  if (distanceMm > 125u) return 5u;

  const uint32_t span = 125u - FullDistanceMm;
  // Round toward the next lower 5% mark; 100% requires reaching the full level.
  const uint32_t steps = ((distanceMm - FullDistanceMm) * 18u + span - 1u) / span;
  return 100u - steps * 5u;
}

#endif
