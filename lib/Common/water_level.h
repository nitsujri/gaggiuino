#ifndef WATER_LEVEL_H
#define WATER_LEVEL_H

#include <stdint.h>

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
