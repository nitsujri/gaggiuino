#include "indigo/fields.h"
#include <cmath>
#include <cstring>
namespace Indigo {
const Field profileFields[] = {
    {"setpoint", "Temperature C", offsetof(eepromValues_t::profile_t, setpoint), Kind::U16, 0, 0,
     Role::Value},
    {"stopOnWeightState", "Stop on weight", offsetof(eepromValues_t::profile_t, stopOnWeightState),
     Kind::Boolean, 0, 0, Role::Enable},
    {"shotDose", "Dose g", offsetof(eepromValues_t::profile_t, shotDose), Kind::Float, 0, 0, Role::Value},
    {"shotStopOnCustomWeight", "Stop weight g", offsetof(eepromValues_t::profile_t, shotStopOnCustomWeight),
     Kind::Float, 0, 0, Role::Value},
    {"preinfusionState", "Preinfusion enabled", offsetof(eepromValues_t::profile_t, preinfusionState),
     Kind::Boolean, 1, 0, Role::Enable},
    {"preinfusionFlowState", "Use flow control", offsetof(eepromValues_t::profile_t, preinfusionFlowState),
     Kind::Boolean, 1, 0, Role::Mode},
    {"preinfusionSec", "Time s", offsetof(eepromValues_t::profile_t, preinfusionSec), Kind::U16, 1, 1,
     Role::Value},
    {"preinfusionBar", "Pressure bar", offsetof(eepromValues_t::profile_t, preinfusionBar), Kind::Float, 1, 1,
     Role::Value},
    {"preinfusionFlowVol", "Flow ml/s", offsetof(eepromValues_t::profile_t, preinfusionFlowVol), Kind::Float,
     1, 2, Role::Value},
    {"preinfusionFlowTime", "Time s", offsetof(eepromValues_t::profile_t, preinfusionFlowTime), Kind::U16, 1,
     2, Role::Value},
    {"preinfusionFlowPressureTarget", "Limit bar",
     offsetof(eepromValues_t::profile_t, preinfusionFlowPressureTarget), Kind::Float, 1, 2, Role::Value},
    {"preinfusionPressureFlowTarget", "Limit ml/s",
     offsetof(eepromValues_t::profile_t, preinfusionPressureFlowTarget), Kind::Float, 1, 1, Role::Value},
    {"preinfusionFilled", "Pumped exit ml", offsetof(eepromValues_t::profile_t, preinfusionFilled),
     Kind::Float, 1, 0, Role::Value},
    {"preinfusionPressureAbove", "Pressure exit",
     offsetof(eepromValues_t::profile_t, preinfusionPressureAbove), Kind::Boolean, 1, 0, Role::Value},
    {"preinfusionWeightAbove", "Weight exit g", offsetof(eepromValues_t::profile_t, preinfusionWeightAbove),
     Kind::Float, 1, 0, Role::Value},
    {"soakState", "Soak enabled", offsetof(eepromValues_t::profile_t, soakState), Kind::Boolean, 2, 0,
     Role::Enable},
    {"soakTimePressure", "Time s", offsetof(eepromValues_t::profile_t, soakTimePressure), Kind::U16, 2, 1,
     Role::Value},
    {"soakTimeFlow", "Time s", offsetof(eepromValues_t::profile_t, soakTimeFlow), Kind::U16, 2, 2,
     Role::Value},
    {"soakKeepPressure", "Hold bar", offsetof(eepromValues_t::profile_t, soakKeepPressure), Kind::Float, 2, 0,
     Role::Value},
    {"soakKeepFlow", "Hold ml/s", offsetof(eepromValues_t::profile_t, soakKeepFlow), Kind::Float, 2, 0,
     Role::Value},
    {"soakBelowPressure", "Exit below bar", offsetof(eepromValues_t::profile_t, soakBelowPressure),
     Kind::Float, 2, 0, Role::Value},
    {"soakAbovePressure", "Exit above bar", offsetof(eepromValues_t::profile_t, soakAbovePressure),
     Kind::Float, 2, 0, Role::Value},
    {"soakAboveWeight", "Weight exit g", offsetof(eepromValues_t::profile_t, soakAboveWeight), Kind::Float, 2,
     0, Role::Value},
    {"preinfusionRamp", "Ramp time s", offsetof(eepromValues_t::profile_t, preinfusionRamp), Kind::U16, 3, 0,
     Role::Value},
    {"preinfusionRampSlope", "PI ramp curve", offsetof(eepromValues_t::profile_t, preinfusionRampSlope),
     Kind::U16, 3, 0, Role::Curve},
    {"tpState", "Transition enabled", offsetof(eepromValues_t::profile_t, tpState), Kind::Boolean, 4, 0,
     Role::Enable},
    {"tpType", "Use flow transition", offsetof(eepromValues_t::profile_t, tpType), Kind::Boolean, 4, 0,
     Role::Mode},
    {"tpProfilingStart", "Start bar", offsetof(eepromValues_t::profile_t, tpProfilingStart), Kind::Float, 4,
     1, Role::Value},
    {"tpProfilingFinish", "End bar", offsetof(eepromValues_t::profile_t, tpProfilingFinish), Kind::Float, 4,
     1, Role::Value},
    {"tpProfilingHold", "Hold s", offsetof(eepromValues_t::profile_t, tpProfilingHold), Kind::U16, 4, 1,
     Role::Value},
    {"tpProfilingHoldLimit", "Hold cap ml/s", offsetof(eepromValues_t::profile_t, tpProfilingHoldLimit),
     Kind::Float, 4, 1, Role::Value},
    {"tpProfilingSlope", "Ramp time s", offsetof(eepromValues_t::profile_t, tpProfilingSlope), Kind::U16, 4,
     1, Role::Value},
    {"tpProfilingSlopeShape", "Pressure ramp curve",
     offsetof(eepromValues_t::profile_t, tpProfilingSlopeShape), Kind::U16, 4, 1, Role::Curve},
    {"tpProfilingFlowRestriction", "Ramp cap ml/s",
     offsetof(eepromValues_t::profile_t, tpProfilingFlowRestriction), Kind::Float, 4, 1, Role::Value},
    {"tfProfileStart", "Start ml/s", offsetof(eepromValues_t::profile_t, tfProfileStart), Kind::Float, 4, 2,
     Role::Value},
    {"tfProfileEnd", "End ml/s", offsetof(eepromValues_t::profile_t, tfProfileEnd), Kind::Float, 4, 2,
     Role::Value},
    {"tfProfileHold", "Hold s", offsetof(eepromValues_t::profile_t, tfProfileHold), Kind::U16, 4, 2,
     Role::Value},
    {"tfProfileHoldLimit", "Hold cap bar", offsetof(eepromValues_t::profile_t, tfProfileHoldLimit),
     Kind::Float, 4, 2, Role::Value},
    {"tfProfileSlope", "Ramp time s", offsetof(eepromValues_t::profile_t, tfProfileSlope), Kind::U16, 4, 2,
     Role::Value},
    {"tfProfileSlopeShape", "Flow ramp curve", offsetof(eepromValues_t::profile_t, tfProfileSlopeShape),
     Kind::U16, 4, 2, Role::Curve},
    {"tfProfilingPressureRestriction", "Ramp cap bar",
     offsetof(eepromValues_t::profile_t, tfProfilingPressureRestriction), Kind::Float, 4, 2, Role::Value},
    {"profilingState", "Brew profiling enabled", offsetof(eepromValues_t::profile_t, profilingState),
     Kind::Boolean, 5, 0, Role::Enable},
    {"mfProfileState", "Use flow profiling", offsetof(eepromValues_t::profile_t, mfProfileState),
     Kind::Boolean, 5, 0, Role::Mode},
    {"mpProfilingStart", "Start bar", offsetof(eepromValues_t::profile_t, mpProfilingStart), Kind::Float, 5,
     1, Role::Value},
    {"mpProfilingFinish", "End bar", offsetof(eepromValues_t::profile_t, mpProfilingFinish), Kind::Float, 5,
     1, Role::Value},
    {"mpProfilingSlope", "Ramp time s", offsetof(eepromValues_t::profile_t, mpProfilingSlope), Kind::U16, 5,
     1, Role::Value},
    {"mpProfilingSlopeShape", "Pressure ramp curve",
     offsetof(eepromValues_t::profile_t, mpProfilingSlopeShape), Kind::U16, 5, 1, Role::Curve},
    {"mpProfilingFlowRestriction", "Limit ml/s",
     offsetof(eepromValues_t::profile_t, mpProfilingFlowRestriction), Kind::Float, 5, 1, Role::Value},
    {"mfProfileStart", "Start ml/s", offsetof(eepromValues_t::profile_t, mfProfileStart), Kind::Float, 5, 2,
     Role::Value},
    {"mfProfileEnd", "End ml/s", offsetof(eepromValues_t::profile_t, mfProfileEnd), Kind::Float, 5, 2,
     Role::Value},
    {"mfProfileSlope", "Ramp time s", offsetof(eepromValues_t::profile_t, mfProfileSlope), Kind::U16, 5, 2,
     Role::Value},
    {"mfProfileSlopeShape", "Flow ramp curve", offsetof(eepromValues_t::profile_t, mfProfileSlopeShape),
     Kind::U16, 5, 2, Role::Curve},
    {"mfProfilingPressureRestriction", "Limit bar",
     offsetof(eepromValues_t::profile_t, mfProfilingPressureRestriction), Kind::Float, 5, 2, Role::Value},
    {"shotRatioX100", "Ratio 1:x", offsetof(eepromValues_t::profile_t, shotRatioX100), Kind::U16, 0, 0,
     Role::Value},
};
const unsigned profileFieldCount = sizeof(profileFields) / sizeof(*profileFields);
const Field systemFields[] = {
    {"steamSetPoint", "Steam temp C", offsetof(eepromValues_t, steamSetPoint), Kind::U16, 0, 0, Role::Value},
    {"offsetTemp", "Temp offset C", offsetof(eepromValues_t, offsetTemp), Kind::U16, 0, 0, Role::Value},
    {"hpwr", "Heater pulse ms", offsetof(eepromValues_t, hpwr), Kind::U16, 0, 0, Role::Value},
    {"mainDivider", "Idle divider", offsetof(eepromValues_t, mainDivider), Kind::U16, 0, 0, Role::Value},
    {"brewDivider", "Brew divider", offsetof(eepromValues_t, brewDivider), Kind::U16, 0, 0, Role::Value},
    {"scalesF1", "Cell 1 factor", offsetof(eepromValues_t, scalesF1), Kind::I32, 1, 0, Role::Value},
    {"scalesF2", "Cell 2 factor", offsetof(eepromValues_t, scalesF2), Kind::I32, 1, 0, Role::Value},
    {"pumpFlowAtZero", "Pump zero", offsetof(eepromValues_t, pumpFlowAtZero), Kind::Float, 1, 0, Role::Value},
    {"lcdSleep", "Sleep minutes", offsetof(eepromValues_t, lcdSleep), Kind::U16, 2, 0, Role::Value},
    {"warmupState", "Warmup", offsetof(eepromValues_t, warmupState), Kind::Boolean, 2, 0, Role::Value},
    {"brightness", "Brightness %", offsetof(eepromValues_t, brightness), Kind::Byte, 2, 0, Role::Value},
    {"homeOnShotFinish", "Home after shot", offsetof(eepromValues_t, homeOnShotFinish), Kind::Boolean, 3, 0,
     Role::Value},
    {"brewDeltaState", "Brew delta", offsetof(eepromValues_t, brewDeltaState), Kind::Boolean, 3, 0,
     Role::Value},
    {"basketPrefill", "Basket prefill", offsetof(eepromValues_t, basketPrefill), Kind::Boolean, 3, 0,
     Role::Value},
    {"powerLineFrequency", "Mains Hz", offsetof(eepromValues_t, powerLineFrequency), Kind::U16, 4, 0,
     Role::Value},
    {"ledState", "LED enabled", offsetof(eepromValues_t, ledState), Kind::Boolean, 4, 0, Role::Value},
    {"ledDisco", "LED effects", offsetof(eepromValues_t, ledDisco), Kind::Boolean, 4, 0, Role::Value},
    {"ledR", "Red", offsetof(eepromValues_t, ledR), Kind::Byte, 4, 0, Role::Value},
    {"ledG", "Green", offsetof(eepromValues_t, ledG), Kind::Byte, 4, 0, Role::Value},
    {"ledB", "Blue", offsetof(eepromValues_t, ledB), Kind::Byte, 4, 0, Role::Value},
    {"tankFullDistanceMm", "Full distance mm", offsetof(eepromValues_t, tankFullDistanceMm), Kind::U16, 5, 0,
     Role::Value},
    {"trayMinWeightG", "Tray minimum g", offsetof(eepromValues_t, trayMinWeightG), Kind::Float, 5, 0,
     Role::Value},
};
const unsigned systemFieldCount = sizeof(systemFields) / sizeof(*systemFields);

double readField(const void *record, const Field &f) {
  const auto *p = static_cast<const uint8_t *>(record) + f.offset;
  switch (f.kind) {
  case Kind::Boolean:
    return *reinterpret_cast<const bool *>(p);
  case Kind::Byte:
    return *p;
  case Kind::U16:
    return *reinterpret_cast<const uint16_t *>(p);
  case Kind::I32:
    return *reinterpret_cast<const int *>(p);
  case Kind::Float:
    return *reinterpret_cast<const float *>(p);
  default:
    return 0;
  }
}
bool writeField(void *record, const Field &f, double value) {
  if (!std::isfinite(value) || f.kind == Kind::Name)
    return false;
  const bool signedFactor = std::strcmp(f.key, "scalesF1") == 0 || std::strcmp(f.key, "scalesF2") == 0;
  if (signedFactor) {
    if (value == 0 || value < -20000 || value > 20000)
      return false;
  } else if (value < 0)
    return false;
  if (f.kind != Kind::Float && std::floor(value) != value)
    return false;
  if (f.kind == Kind::U16 && value > 65535)
    return false;
  if (f.kind == Kind::Byte && value > 255)
    return false;
  if (f.kind == Kind::Boolean && value != 0 && value != 1)
    return false;
  if ((std::strcmp(f.key, "setpoint") == 0 || std::strcmp(f.key, "steamSetPoint") == 0) &&
      (value < 1 || value > 165))
    return false;
  if ((std::strcmp(f.key, "mainDivider") == 0 || std::strcmp(f.key, "brewDivider") == 0 ||
       std::strcmp(f.key, "hpwr") == 0) &&
      value < 1)
    return false;
  if (std::strcmp(f.key, "pumpFlowAtZero") == 0 && (value < .210 || value > .310))
    return false;
  if (std::strcmp(f.key, "powerLineFrequency") == 0 && value != 50 && value != 60)
    return false;
  if (std::strcmp(f.key, "tankFullDistanceMm") == 0 && (value < 1 || value >= 125))
    return false;
  if (std::strcmp(f.key, "brightness") == 0 && (value < 1 || value > 100))
    return false;
  if ((std::strcmp(f.key, "shotDose") == 0 || std::strcmp(f.key, "shotRatioX100") == 0 ||
       std::strcmp(f.key, "trayMinWeightG") == 0) &&
      value <= 0)
    return false;
  if (f.role == Role::Curve && value > 4)
    return false;
  auto *p = static_cast<uint8_t *>(record) + f.offset;
  switch (f.kind) {
  case Kind::Boolean:
    *reinterpret_cast<bool *>(p) = value;
    break;
  case Kind::Byte:
    *p = value;
    break;
  case Kind::U16:
    *reinterpret_cast<uint16_t *>(p) = value;
    break;
  case Kind::I32:
    *reinterpret_cast<int *>(p) = value;
    break;
  case Kind::Float:
    if (value > 1.e6)
      return false;
    *reinterpret_cast<float *>(p) = value;
    break;
  default:
    return false;
  }
  return true;
}
} // namespace Indigo
