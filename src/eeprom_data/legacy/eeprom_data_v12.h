#pragma once
#include "../eeprom_data.h"
#include <cstring>

// Frozen schema 12. Do not change: these bytes are already on installed boards.
struct eepromValues_t_v12 {
  uint16_t steamSetPoint;
  uint16_t offsetTemp;
  uint16_t hpwr;
  uint16_t mainDivider;
  uint16_t brewDivider;
  uint8_t activeProfile;
  struct profile_t {
    char name[PROFILE_NAME_LENGTH];
    // Preinfusion vars section
    bool preinfusionState;
    bool preinfusionFlowState;
    uint16_t preinfusionSec;
    float preinfusionBar;
    float preinfusionFlowVol;
    uint16_t preinfusionFlowTime;
    float preinfusionFlowPressureTarget;
    float preinfusionPressureFlowTarget;
    float preinfusionFilled;
    bool preinfusionPressureAbove;
    float preinfusionWeightAbove;
    // Soak vars section
    bool soakState;
    uint16_t soakTimePressure;
    uint16_t soakTimeFlow;
    float soakKeepPressure;
    float soakKeepFlow;
    float soakBelowPressure;
    float soakAbovePressure;
    float soakAboveWeight;
    // PI -> PF ramp settings
    uint16_t preinfusionRamp;
    uint16_t preinfusionRampSlope;
    // Profiling vars section
    bool tpState;
    bool tpType;
    float tpProfilingStart;
    float tpProfilingFinish;
    uint16_t tpProfilingHold;
    float tpProfilingHoldLimit;
    uint16_t tpProfilingSlope;
    uint16_t tpProfilingSlopeShape;
    float tpProfilingFlowRestriction;
    float tfProfileStart;
    float tfProfileEnd;
    uint16_t tfProfileHold;
    float tfProfileHoldLimit;
    uint16_t tfProfileSlope;
    uint16_t tfProfileSlopeShape;
    float tfProfilingPressureRestriction;
    bool profilingState;
    bool mfProfileState;
    float mpProfilingStart;
    float mpProfilingFinish;
    uint16_t mpProfilingSlope;
    uint16_t mpProfilingSlopeShape;
    float mpProfilingFlowRestriction;
    float mfProfileStart;
    float mfProfileEnd;
    uint16_t mfProfileSlope;
    uint16_t mfProfileSlopeShape;
    float mfProfilingPressureRestriction;
    /*-----------OTHER---------------------*/
    uint16_t setpoint;
    bool stopOnWeightState;
    float shotDose;
    float shotStopOnCustomWeight;
    uint16_t shotPreset;
  } profiles[MAX_PROFILES];
  // Settings vars section
  uint16_t powerLineFrequency;
  uint16_t lcdSleep;
  bool warmupState;
  bool homeOnShotFinish;
  bool brewDeltaState;
  bool basketPrefill;
  int scalesF1;
  int scalesF2;
  float pumpFlowAtZero;
  bool ledState;
  bool ledDisco;
  uint8_t ledR;
  uint8_t ledG;
  uint8_t ledB;
};

static_assert(sizeof(eepromValues_t_v12::profile_t) == 200, "v12 profile layout");
static_assert(sizeof(eepromValues_t_v12) == 1040, "v12 config layout");

inline bool upgradeSchema_v12(eepromValues_t &target, const eepromValues_t_v12 &source) {
  target.steamSetPoint = source.steamSetPoint;
  target.offsetTemp = source.offsetTemp;
  target.hpwr = source.hpwr;
  target.mainDivider = source.mainDivider;
  target.brewDivider = source.brewDivider;
  target.activeProfile = source.activeProfile;
  target.powerLineFrequency = source.powerLineFrequency;
  target.lcdSleep = source.lcdSleep;
  target.warmupState = source.warmupState;
  target.homeOnShotFinish = source.homeOnShotFinish;
  target.brewDeltaState = source.brewDeltaState;
  target.basketPrefill = source.basketPrefill;
  target.scalesF1 = source.scalesF1;
  target.scalesF2 = source.scalesF2;
  target.pumpFlowAtZero = source.pumpFlowAtZero;
  target.ledState = source.ledState;
  target.ledDisco = source.ledDisco;
  target.ledR = source.ledR;
  target.ledG = source.ledG;
  target.ledB = source.ledB;
  for (unsigned i = 0; i < 5; i++) {
    std::memcpy(target.profiles[i].name, source.profiles[i].name, 25);
    target.profiles[i].preinfusionState = source.profiles[i].preinfusionState;
    target.profiles[i].preinfusionFlowState = source.profiles[i].preinfusionFlowState;
    target.profiles[i].preinfusionSec = source.profiles[i].preinfusionSec;
    target.profiles[i].preinfusionBar = source.profiles[i].preinfusionBar;
    target.profiles[i].preinfusionFlowVol = source.profiles[i].preinfusionFlowVol;
    target.profiles[i].preinfusionFlowTime = source.profiles[i].preinfusionFlowTime;
    target.profiles[i].preinfusionFlowPressureTarget = source.profiles[i].preinfusionFlowPressureTarget;
    target.profiles[i].preinfusionPressureFlowTarget = source.profiles[i].preinfusionPressureFlowTarget;
    target.profiles[i].preinfusionFilled = source.profiles[i].preinfusionFilled;
    target.profiles[i].preinfusionPressureAbove = source.profiles[i].preinfusionPressureAbove;
    target.profiles[i].preinfusionWeightAbove = source.profiles[i].preinfusionWeightAbove;
    target.profiles[i].soakState = source.profiles[i].soakState;
    target.profiles[i].soakTimePressure = source.profiles[i].soakTimePressure;
    target.profiles[i].soakTimeFlow = source.profiles[i].soakTimeFlow;
    target.profiles[i].soakKeepPressure = source.profiles[i].soakKeepPressure;
    target.profiles[i].soakKeepFlow = source.profiles[i].soakKeepFlow;
    target.profiles[i].soakBelowPressure = source.profiles[i].soakBelowPressure;
    target.profiles[i].soakAbovePressure = source.profiles[i].soakAbovePressure;
    target.profiles[i].soakAboveWeight = source.profiles[i].soakAboveWeight;
    target.profiles[i].preinfusionRamp = source.profiles[i].preinfusionRamp;
    target.profiles[i].preinfusionRampSlope = source.profiles[i].preinfusionRampSlope;
    target.profiles[i].tpState = source.profiles[i].tpState;
    target.profiles[i].tpType = source.profiles[i].tpType;
    target.profiles[i].tpProfilingStart = source.profiles[i].tpProfilingStart;
    target.profiles[i].tpProfilingFinish = source.profiles[i].tpProfilingFinish;
    target.profiles[i].tpProfilingHold = source.profiles[i].tpProfilingHold;
    target.profiles[i].tpProfilingHoldLimit = source.profiles[i].tpProfilingHoldLimit;
    target.profiles[i].tpProfilingSlope = source.profiles[i].tpProfilingSlope;
    target.profiles[i].tpProfilingSlopeShape = source.profiles[i].tpProfilingSlopeShape;
    target.profiles[i].tpProfilingFlowRestriction = source.profiles[i].tpProfilingFlowRestriction;
    target.profiles[i].tfProfileStart = source.profiles[i].tfProfileStart;
    target.profiles[i].tfProfileEnd = source.profiles[i].tfProfileEnd;
    target.profiles[i].tfProfileHold = source.profiles[i].tfProfileHold;
    target.profiles[i].tfProfileHoldLimit = source.profiles[i].tfProfileHoldLimit;
    target.profiles[i].tfProfileSlope = source.profiles[i].tfProfileSlope;
    target.profiles[i].tfProfileSlopeShape = source.profiles[i].tfProfileSlopeShape;
    target.profiles[i].tfProfilingPressureRestriction = source.profiles[i].tfProfilingPressureRestriction;
    target.profiles[i].profilingState = source.profiles[i].profilingState;
    target.profiles[i].mfProfileState = source.profiles[i].mfProfileState;
    target.profiles[i].mpProfilingStart = source.profiles[i].mpProfilingStart;
    target.profiles[i].mpProfilingFinish = source.profiles[i].mpProfilingFinish;
    target.profiles[i].mpProfilingSlope = source.profiles[i].mpProfilingSlope;
    target.profiles[i].mpProfilingSlopeShape = source.profiles[i].mpProfilingSlopeShape;
    target.profiles[i].mpProfilingFlowRestriction = source.profiles[i].mpProfilingFlowRestriction;
    target.profiles[i].mfProfileStart = source.profiles[i].mfProfileStart;
    target.profiles[i].mfProfileEnd = source.profiles[i].mfProfileEnd;
    target.profiles[i].mfProfileSlope = source.profiles[i].mfProfileSlope;
    target.profiles[i].mfProfileSlopeShape = source.profiles[i].mfProfileSlopeShape;
    target.profiles[i].mfProfilingPressureRestriction = source.profiles[i].mfProfilingPressureRestriction;
    target.profiles[i].setpoint = source.profiles[i].setpoint;
    target.profiles[i].stopOnWeightState = source.profiles[i].stopOnWeightState;
    target.profiles[i].shotDose = source.profiles[i].shotDose;
    target.profiles[i].shotStopOnCustomWeight = source.profiles[i].shotStopOnCustomWeight;
    target.profiles[i].shotPreset = source.profiles[i].shotPreset;
    target.profiles[i].shotRatioX100 = source.profiles[i].shotPreset * 100u;
  }
  return source.activeProfile < 5;
}
