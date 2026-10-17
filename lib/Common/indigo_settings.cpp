#include "indigo/settings.h"
bool settingsValid(const eepromValues_t &c) {
  if (c.activeProfile >= 5 || c.steamSetPoint < 1 || c.steamSetPoint > 165 || c.mainDivider < 1 ||
      c.brewDivider < 1 || c.hpwr < 1)
    return false;
  if (c.powerLineFrequency != 50 && c.powerLineFrequency != 60)
    return false;
  if (!std::isfinite(c.pumpFlowAtZero) || c.pumpFlowAtZero < .210f || c.pumpFlowAtZero > .310f)
    return false;
  if (c.scalesF1 == 0 || c.scalesF1 < -20000 || c.scalesF1 > 20000 || c.scalesF2 == 0 ||
      c.scalesF2 < -20000 || c.scalesF2 > 20000)
    return false;
  if (c.tankFullDistanceMm < 1 || c.tankFullDistanceMm >= 125 || c.brightness < 1 || c.brightness > 100 ||
      !std::isfinite(c.trayMinWeightG) || c.trayMinWeightG <= 0)
    return false;
  for (const auto &p : c.profiles) {
    if (!p.name[0] || !std::memchr(p.name, 0, 25) || p.setpoint < 1 || p.setpoint > 165 || p.shotDose <= 0 ||
        (!p.shotRatioX100 && !p.shotPreset))
      return false;
    if (!std::isfinite(p.preinfusionBar) || p.preinfusionBar < 0.f)
      return false;
    if (!std::isfinite(p.preinfusionFlowVol) || p.preinfusionFlowVol < 0.f)
      return false;
    if (!std::isfinite(p.preinfusionFlowPressureTarget) || p.preinfusionFlowPressureTarget < 0.f)
      return false;
    if (!std::isfinite(p.preinfusionPressureFlowTarget) || p.preinfusionPressureFlowTarget < 0.f)
      return false;
    if (!std::isfinite(p.preinfusionFilled) || p.preinfusionFilled < 0.f)
      return false;
    if (!std::isfinite(p.preinfusionWeightAbove) || p.preinfusionWeightAbove < 0.f)
      return false;
    if (!std::isfinite(p.soakKeepPressure) || p.soakKeepPressure < 0.f)
      return false;
    if (!std::isfinite(p.soakKeepFlow) || p.soakKeepFlow < 0.f)
      return false;
    if (!std::isfinite(p.soakBelowPressure) || p.soakBelowPressure < 0.f)
      return false;
    if (!std::isfinite(p.soakAbovePressure) || p.soakAbovePressure < 0.f)
      return false;
    if (!std::isfinite(p.soakAboveWeight) || p.soakAboveWeight < 0.f)
      return false;
    if (!std::isfinite(p.tpProfilingStart) || p.tpProfilingStart < 0.f)
      return false;
    if (!std::isfinite(p.tpProfilingFinish) || p.tpProfilingFinish < 0.f)
      return false;
    if (!std::isfinite(p.tpProfilingHoldLimit) || p.tpProfilingHoldLimit < 0.f)
      return false;
    if (!std::isfinite(p.tpProfilingFlowRestriction) || p.tpProfilingFlowRestriction < 0.f)
      return false;
    if (!std::isfinite(p.tfProfileStart) || p.tfProfileStart < 0.f)
      return false;
    if (!std::isfinite(p.tfProfileEnd) || p.tfProfileEnd < 0.f)
      return false;
    if (!std::isfinite(p.tfProfileHoldLimit) || p.tfProfileHoldLimit < 0.f)
      return false;
    if (!std::isfinite(p.tfProfilingPressureRestriction) || p.tfProfilingPressureRestriction < 0.f)
      return false;
    if (!std::isfinite(p.mpProfilingStart) || p.mpProfilingStart < 0.f)
      return false;
    if (!std::isfinite(p.mpProfilingFinish) || p.mpProfilingFinish < 0.f)
      return false;
    if (!std::isfinite(p.mpProfilingFlowRestriction) || p.mpProfilingFlowRestriction < 0.f)
      return false;
    if (!std::isfinite(p.mfProfileStart) || p.mfProfileStart < 0.f)
      return false;
    if (!std::isfinite(p.mfProfileEnd) || p.mfProfileEnd < 0.f)
      return false;
    if (!std::isfinite(p.mfProfilingPressureRestriction) || p.mfProfilingPressureRestriction < 0.f)
      return false;
    if (!std::isfinite(p.shotDose) || p.shotDose < 0.f)
      return false;
    if (!std::isfinite(p.shotStopOnCustomWeight) || p.shotStopOnCustomWeight < 0.f)
      return false;
    if (p.preinfusionRampSlope > 4)
      return false;
    if (p.tpProfilingSlopeShape > 4)
      return false;
    if (p.tfProfileSlopeShape > 4)
      return false;
    if (p.mpProfilingSlopeShape > 4)
      return false;
    if (p.mfProfileSlopeShape > 4)
      return false;
  }
  return true;
}
