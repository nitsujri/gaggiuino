#pragma once
#include "../../../src/eeprom_data/eeprom_data.h"
#include <cmath>
#include <cstring>
#include <stdint.h>

inline uint32_t settingsChecksum(const void *bytes, size_t size) {
  const auto *p = static_cast<const uint8_t *>(bytes);
  uint32_t crc = 0xffffffffu;
  while (size--) {
    crc ^= *p++;
    for (unsigned b = 0; b < 8; b++)
      crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320u : 0u);
  }
  return ~crc;
}
inline float profileStopTarget(const eepromValues_t::profile_t &p) {
  return p.shotStopOnCustomWeight > 0.f
             ? p.shotStopOnCustomWeight
             : p.shotDose * (p.shotRatioX100 ? p.shotRatioX100 / 100.f : p.shotPreset);
}
inline uint16_t tankPercent(uint16_t mm, uint16_t full) {
  if (full < 1 || full >= 125)
    full = 30;
  if (mm > 125)
    return 9;
  if (mm <= full)
    return 100;
  return static_cast<uint16_t>(std::lround(10.f + 90.f * (125 - mm) / (125 - full)));
}
bool settingsValid(const eepromValues_t &cfg);

inline eepromValues_t profileSaveValues(const eepromValues_t &saved, const eepromValues_t &running,
                                        unsigned index) {
  auto merged = saved;
  if (index < 5)
    merged.profiles[index] = running.profiles[index];
  return merged;
}
inline eepromValues_t systemSaveValues(const eepromValues_t &saved, const eepromValues_t &running) {
  auto merged = running;
  for (unsigned i = 0; i < 5; i++)
    merged.profiles[i] = saved.profiles[i];
  merged.activeProfile = saved.activeProfile;
  return merged;
}

inline bool initializeIndigoUpgrade(eepromValues_t &values, const eepromValues_t &defaults,
                                    unsigned version) {
  if (version >= 13)
    return false;
  values.profiles[0] = defaults.profiles[0];
  values.profiles[1] = defaults.profiles[1];
  values.activeProfile = 0;
  return true;
}
