/* 09:32 15/03/2023 - change triggering comment */
// Keep project O3 semantics: Ofast may optimize away non-finite sensor checks.
#include <cmath>
#if defined(DEBUG_ENABLED)
  #include "dbg.h"
#endif
#include "gaggiuino.h"
#include "lcd/indigo.h"
#include "indigo/settings.h"
#include "indigo/control.h"

SimpleKalmanFilter smoothPressure(0.6f, 0.6f, 0.1f);
SimpleKalmanFilter smoothPumpFlow(0.1f, 0.1f, 0.01f);
SimpleKalmanFilter smoothScalesFlow(0.5f, 0.5f, 0.01f);
SimpleKalmanFilter smoothConsideredFlow(0.1f, 0.1f, 0.1f);

//default phases. Updated in updateProfilerPhases.
Profile profile;
PhaseProfiler phaseProfiler{profile};

PredictiveWeight predictiveWeight;

SensorState currentState;

OPERATION_MODES selectedOperationalMode;

eepromValues_t runningCfg;

SystemState systemState;

LED led;
TOF tof;
static bool homeFlush=false,manualMode=false;
static int cleaningMode=0;
static bool indigoCommand(Indigo::Command,int);
static void indigoUpdate();
static void updateProfilerPhases();
static bool dripTrayAllowsPressureRelease();

void setup(void) {
  LOG_INIT();
  LOG_INFO("Gaggiuino (fw: %s) booting", AUTO_VERSION);

  // Various pins operation mode handling
  pinInit();
  LOG_INFO("Pin init");

  setBoilerOff();  // relayPin LOW
  setSteamValveRelayOff();
  setSteamBoilerRelayOff();
  LOG_INFO("Boiler turned off");

  //Pump
  setPumpOff();
  LOG_INFO("Pump turned off");

  // Valve
  closeValve();
  LOG_INFO("Valve closed");

  lcdInit();
  LOG_INFO("LCD Init");

#if defined(DEBUG_ENABLED)
  // Debug init if enabled
  dbgInit();
  LOG_INFO("DBG init");
#endif

  // Initialise comms library for talking to the ESP mcu
  espCommsInit();

  // Initialize LED
  led.begin();
  led.setColor(9u, 0u, 9u); // WHITE
  // Init the tof sensor
  tof.init(currentState);

  // Initialising the saved values or writing defaults if first start
  eepromInit();
  runningCfg = eepromGetCurrentValues();
  LOG_INFO("EEPROM Init");

  cpsInit(runningCfg);
  LOG_INFO("CPS Init");

  thermocoupleInit();
  LOG_INFO("Thermocouple Init");

  lcdUploadCfg(runningCfg);
  LOG_INFO("LCD cfg uploaded");

  adsInit();
  LOG_INFO("Pressure sensor init");

  // Scales handling
  scalesInit(runningCfg.scalesF1, runningCfg.scalesF2);
  scalesConfigure(runningCfg);
  LOG_INFO("Scales init");

  // Pump init
  pumpInit(runningCfg.powerLineFrequency, runningCfg.pumpFlowAtZero);
  LOG_INFO("Pump init");

  pageValuesRefresh();
  LOG_INFO("Setup sequence finished");

  // Change LED colour on setup exit.
  led.setColor(9u, 0u, 9u); // 64171

  iwdcInit();
  indigoBegin(runningCfg,currentState,indigoCommand);
}

//##############################################################################################################################
//############################################________________MAIN______________################################################
//##############################################################################################################################


//Main loop where all the logic is continuously run
void loop(void) {
  if (lcdCurrentPageId != lcdLastCurrentPageId) pageValuesRefresh();
  lcdListen();
  sensorsRead();
  if(!std::isfinite(currentState.temperature)||currentState.temperature<=0.f||currentState.temperature>=170.f){sysHealthCheck(SYS_PRESSURE_IDLE);return;}
  fillBoiler();
  brewDetect();
  modeSelect();
  lcdRefresh();
  espCommsSendSensorData(currentState);
  sysHealthCheck(SYS_PRESSURE_IDLE);
  if(indigoActive())indigoUpdate();
}

//##############################################################################################################################
//#############################################___________SENSORS_READ________##################################################
//##############################################################################################################################


static void sensorsRead(void) {
  sensorReadSwitches();
  espCommsReadData();
  sensorsReadTemperature();
  sensorsReadWeight();
  sensorsReadPressure();
  calculateWeightAndFlow();
  updateStartupTimer();
  readTankWaterLevel();
  doLed();
}

static void sensorReadSwitches(void) {
  currentState.brewSwitchState = brewState();
  currentState.steamSwitchState = steamState();
  currentState.hotWaterSwitchState = waterPinState() || (currentState.brewSwitchState && currentState.steamSwitchState); // use either an actual switch, or the GC/GCP switch combo
}

static void sensorsReadTemperature(void) {
  if (millis() > thermoTimer) {
    currentState.temperature = thermocoupleRead() - runningCfg.offsetTemp;
    thermoTimer = millis() + GET_KTYPE_READ_EVERY;
  }
}

static void sensorsReadWeight(void) {
  scalesPoll();
  uint32_t elapsedTime = millis() - scalesTimer;

  if (elapsedTime > GET_SCALES_READ_EVERY) {
    currentState.scalesPresent = scalesIsPresent();
    if (currentState.scalesPresent) {
      if (currentState.tarePending) {
        scalesTare();
        weightMeasurements.clear();
        weightMeasurements.add(scalesGetWeight());
        currentState.tarePending = false;
      }
      else {
        weightMeasurements.add(scalesGetWeight());
      }
      currentState.weight = weightMeasurements.latest().value;

      if (brewActive) {
        currentState.shotWeight = currentState.tarePending ? 0.f : currentState.weight;
        currentState.weightFlow = fmax(0.f, weightMeasurements.measurementChange().changeSpeed());
        currentState.smoothedWeightFlow = smoothScalesFlow.updateEstimate(currentState.weightFlow);
      }
    }
    scalesTimer = millis();
  }
}

static void sensorsReadPressure(void) {
  uint32_t elapsedTime = millis() - pressureTimer;

  if (elapsedTime > GET_PRESSURE_READ_EVERY) {
    float elapsedTimeSec = elapsedTime / 1000.f;
    currentState.pressure = getPressure();
    previousSmoothedPressure = currentState.smoothedPressure;
    currentState.smoothedPressure = smoothPressure.updateEstimate(currentState.pressure);
    currentState.pressureChangeSpeed = (currentState.smoothedPressure - previousSmoothedPressure) / elapsedTimeSec;
    pressureTimer = millis();
  }
}

static long sensorsReadFlow(float elapsedTimeSec) {
  long pumpClicks = getAndResetClickCounter();
  currentState.pumpClicks = (float) pumpClicks / elapsedTimeSec;

  currentState.pumpFlow = getPumpFlow(currentState.pumpClicks, currentState.smoothedPressure);

  previousSmoothedPumpFlow = currentState.smoothedPumpFlow;
  // Some flow smoothing
  currentState.smoothedPumpFlow = smoothPumpFlow.updateEstimate(currentState.pumpFlow);
  currentState.pumpFlowChangeSpeed = (currentState.smoothedPumpFlow - previousSmoothedPumpFlow) / elapsedTimeSec;
  return pumpClicks;
}

static void calculateWeightAndFlow(void) {
  uint32_t elapsedTime = millis() - flowTimer;

  if (brewActive) {
    // Marking for tare in case smth has gone wrong and it has exited tare already.
    if (currentState.weight < -.3f) currentState.tarePending = true;

    if (elapsedTime > REFRESH_FLOW_EVERY) {
      flowTimer = millis();
      float elapsedTimeSec = elapsedTime / 1000.f;
      long pumpClicks = sensorsReadFlow(elapsedTimeSec);
      float consideredFlow = currentState.smoothedPumpFlow * elapsedTimeSec;
      // Update predictive class with our current phase
      CurrentPhase& phase = phaseProfiler.getCurrentPhase();
      predictiveWeight.update(currentState, phase, runningCfg);

      // Start the predictive weight calculations when conditions are true
      if (predictiveWeight.isOutputFlow() || currentState.weight > 0.4f) {
        float flowPerClick = getPumpFlowPerClick(currentState.smoothedPressure);
        float actualFlow = (consideredFlow > pumpClicks * flowPerClick) ? consideredFlow : pumpClicks * flowPerClick;
        /* Probabilistically the flow is lower if the shot is just started winding up and we're flow profiling,
        once pressure stabilises around the setpoint the flow is either stable or puck restriction is high af. */
        if ((ACTIVE_PROFILE(runningCfg).mfProfileState || ACTIVE_PROFILE(runningCfg).tpType) && currentState.pressureChangeSpeed > 0.15f) {
          if ((currentState.smoothedPressure < ACTIVE_PROFILE(runningCfg).mfProfileStart * 0.9f)
          || (currentState.smoothedPressure < ACTIVE_PROFILE(runningCfg).tfProfileStart * 0.9f)) {
            actualFlow *= 0.3f;
          }
        }
        currentState.consideredFlow = smoothConsideredFlow.updateEstimate(actualFlow);
        currentState.shotWeight = currentState.scalesPresent ? currentState.shotWeight : currentState.shotWeight + actualFlow;
      }
      currentState.waterPumped += consideredFlow;
    }
  } else {
    currentState.consideredFlow = 0.f;
    currentState.pumpClicks = getAndResetClickCounter();
    flowTimer = millis();
  }
}

// return the reading in mm of the tank water level.
static void readTankWaterLevel(void) {
  tof.poll();
#ifdef TOF_VL53L0X
  currentState.waterLvl=tof.readLvl(runningCfg.tankFullDistanceMm);
#else
  currentState.waterLvl=30;
#endif
}

//##############################################################################################################################
//############################################______PAGE_CHANGE_VALUES_REFRESH_____#############################################
//##############################################################################################################################
static void pageValuesRefresh() {
  if(indigoActive()){lcdLastCurrentPageId=lcdCurrentPageId;return;}
  // Read the page we're landing in: leaving keyboard page means a value could've changed in it
  if (lcdLastCurrentPageId == NextionPage::KeyboardNumeric) lcdFetchPage(runningCfg, lcdCurrentPageId, runningCfg.activeProfile);
  // Or maybe it's a page that needs constant polling
  else if (lcdLastCurrentPageId == NextionPage::Led) lcdFetchPage(runningCfg, lcdCurrentPageId, runningCfg.activeProfile);
  // Finally read the page we left, as it could've been changed in place (e.g. boolean toggles)
  else lcdFetchPage(runningCfg, lcdLastCurrentPageId, runningCfg.activeProfile);

  homeScreenScalesEnabled = lcdGetHomeScreenScalesEnabled();
  // MODE_SELECT should always be LAST
  selectedOperationalMode = (OPERATION_MODES) lcdGetSelectedOperationalMode();

  updateProfilerPhases();

  lcdLastCurrentPageId = lcdCurrentPageId;
}

//#############################################################################################
//############################____OPERATIONAL_MODE_CONTROL____#################################
//#############################################################################################
static void modeSelect(void) {
    if(homeFlush){
      const bool allowed=homeFlushAllowed(currentState,millis()-indigoUi()->lastContact());
      if(allowed){nonBrewModeActive=true;flushActivated();setBoilerOff();setSteamValveRelayOff();setSteamBoilerRelayOff();return;}
      homeFlush=false;flushDeactivated();lcdShowPopup("Flush stopped: check tank / switches / display");
    }
  if (!systemState.startupInitFinished) return;
  static bool waterFaultLatch=false;
  const bool switchOn=currentState.brewSwitchState||currentState.steamSwitchState||currentState.hotWaterSwitchState;
  if(currentState.waterLvl<MIN_WATER_LVL){
    waterFaultLatch=waterFaultLatch||switchOn;homeFlush=false;
    setPumpOff();closeValve();setBoilerOff();setSteamValveRelayOff();setSteamBoilerRelayOff();return;
  }
  if(waterFaultLatch){
    if(switchOn){setPumpOff();closeValve();setBoilerOff();setSteamValveRelayOff();setSteamBoilerRelayOff();return;}
    waterFaultLatch=false;
  }
  if(indigoActive()){
    selectedOperationalMode=cleaningMode==1?OPERATION_MODES::OPMODE_flush:cleaningMode==2?OPERATION_MODES::OPMODE_descale:manualMode?OPERATION_MODES::OPMODE_manual:OPERATION_MODES::OPMODE_straight9Bar;

  }

  switch (selectedOperationalMode) {
    //REPLACE ALL THE BELOW WITH OPMODE_auto_profiling
    case OPERATION_MODES::OPMODE_straight9Bar:
    case OPERATION_MODES::OPMODE_justPreinfusion:
    case OPERATION_MODES::OPMODE_justPressureProfile:
    case OPERATION_MODES::OPMODE_preinfusionAndPressureProfile:
    case OPERATION_MODES::OPMODE_flowPreinfusionStraight9BarProfiling:
    case OPERATION_MODES::OPMODE_justFlowBasedProfiling:
    case OPERATION_MODES::OPMODE_FlowBasedPreinfusionPressureBasedProfiling:
    case OPERATION_MODES::OPMODE_everythingFlowProfiled:
    case OPERATION_MODES::OPMODE_pressureBasedPreinfusionAndFlowProfile:
      nonBrewModeActive = false;
      if (currentState.hotWaterSwitchState) hotWaterMode(currentState);
      else if (currentState.steamSwitchState) steamCtrl(runningCfg, currentState);
      else {
        profiling();
        steamTime = millis();
      }
      break;
    case OPERATION_MODES::OPMODE_manual:
      nonBrewModeActive = false;
      if (!currentState.steamSwitchState) steamTime = millis();
      if(currentState.hotWaterSwitchState)hotWaterMode(currentState);
      else if(currentState.steamSwitchState)steamCtrl(runningCfg,currentState);
      else manualFlowControl();
      break;
    case OPERATION_MODES::OPMODE_flush:
      nonBrewModeActive = true;
      if (!currentState.steamSwitchState) steamTime = millis();
      backFlush(currentState);
      brewActive ? setBoilerOff() : justDoCoffee(runningCfg, currentState, false);
      break;
    case OPERATION_MODES::OPMODE_steam:
      nonBrewModeActive = true;
      steamCtrl(runningCfg, currentState);

      if (!currentState.steamSwitchState) {
        (brewActive && dripTrayAllowsPressureRelease()) ? flushActivated() : flushDeactivated();
        steamCtrl(runningCfg, currentState);
        pageValuesRefresh();
      }
      break;
    case OPERATION_MODES::OPMODE_descale:
      nonBrewModeActive = true;
      if (!currentState.steamSwitchState) steamTime = millis();
      deScale(runningCfg, currentState);
      break;
    default:
      pageValuesRefresh();
      break;
  }
}

//#############################################################################################
//################################____LCD_REFRESH_CONTROL___###################################
//#############################################################################################

static void lcdRefresh(void) {
  uint16_t tempDecimal;

  if (millis() > pageRefreshTimer) {
    /*LCD pressure output, as a measure to beautify the graphs locking the live pressure read for the LCD alone*/
    #ifdef BEAUTIFY_GRAPH
      lcdSetPressure(currentState.smoothedPressure * 10.f);
    #else
      lcdSetPressure(
        currentState.pressure > 0.f
          ? currentState.pressure * 10.f
          : 0.f
      );
    #endif

    /*LCD temp output*/
    float brewTempSetPoint = ACTIVE_PROFILE(runningCfg).setpoint + runningCfg.offsetTemp;
    // float liveTempWithOffset = currentState.temperature - runningCfg.offsetTemp;
    currentState.waterTemperature = (currentState.temperature > (float)ACTIVE_PROFILE(runningCfg).setpoint && currentState.brewSwitchState)
      ? currentState.temperature / (float)brewTempSetPoint + (float)ACTIVE_PROFILE(runningCfg).setpoint
      : currentState.temperature;

    lcdSetTemperature(std::floor((uint16_t)currentState.waterTemperature));

    /*LCD weight & temp & water lvl output*/
    switch (lcdCurrentPageId) {
      case NextionPage::Home:
        // temp decimal handling
        tempDecimal = (currentState.waterTemperature - (uint16_t)currentState.waterTemperature) * 10;
        lcdSetTemperatureDecimal(tempDecimal);
        // water lvl
        lcdSetTankWaterLvl(currentState.waterLvl);
        //weight
        if (homeScreenScalesEnabled) lcdSetWeight(currentState.weight);
        break;
      case NextionPage::BrewGraph:
      case NextionPage::BrewManual:
        // temp decimal handling
        tempDecimal = (currentState.waterTemperature - (uint16_t)currentState.waterTemperature) * 10;
        lcdSetTemperatureDecimal(tempDecimal);
        // If the weight output is a negative value lower than -0.8 you might want to tare again before extraction starts.
        if (currentState.shotWeight) lcdSetWeight(currentState.shotWeight > -0.8f ? currentState.shotWeight : -0.9f);
        /*LCD flow output*/
        lcdSetFlow( currentState.smoothedPumpFlow * 10.f);
        break;
      default:
        break; // don't push needless data on other pages
    }

  #ifdef DEBUG_ENABLED
    lcdShowDebug(readTempSensor(), getAdsError());
  #endif

    /*LCD timer and warmup*/
    if (brewActive) {
      lcdSetBrewTimer((millis() > brewingTimer) ? (int)((millis() - brewingTimer) / 1000) : 0);
      lcdBrewTimerStart(); // nextion timer start
      lcdWarmupStateStop(); // Flagging warmup notification on Nextion needs to stop (if enabled)
    } else {
      lcdBrewTimerStop(); // nextion timer stop
    }

    pageRefreshTimer = millis() + REFRESH_SCREEN_EVERY;
  }
}
//#############################################################################################
//###################################____SAVE_BUTTON____#######################################
//#############################################################################################
void tryEepromWrite(const eepromValues_t &eepromValues) {
  bool success = eepromWrite(eepromValues);
  watchdogReload(); // reload the watchdog timer on expensive operations
  if (success) {
    lcdShowPopup("Update successful!");
  } else {
    lcdShowPopup("Data out of range!");
  }
}

void lcdSwitchActiveToStoredProfile(const eepromValues_t & storedSettings) {
  runningCfg.activeProfile = lcdGetSelectedProfile();
  ACTIVE_PROFILE(runningCfg) = storedSettings.profiles[runningCfg.activeProfile];
  updateProfilerPhases();
  lcdUploadProfile(runningCfg);
}

// Save the desired temp values to EEPROM
void lcdSaveSettingsTrigger(void) {
  LOG_VERBOSE("Saving values to EEPROM");

  eepromValues_t eepromCurrentValues = eepromGetCurrentValues();
  lcdFetchPage(eepromCurrentValues, lcdCurrentPageId, runningCfg.activeProfile);
  tryEepromWrite(eepromCurrentValues);
}

void lcdSaveProfileTrigger(void) {
  LOG_VERBOSE("Saving profile to EEPROM");

  eepromValues_t eepromCurrentValues = eepromGetCurrentValues();
  lcdFetchCurrentProfile(eepromCurrentValues);
  tryEepromWrite(eepromCurrentValues);
}

void lcdResetSettingsTrigger(void) {
  tryEepromWrite(eepromGetDefaultValues());
}

void lcdLoadDefaultProfileTrigger(void) {
  lcdSwitchActiveToStoredProfile(eepromGetDefaultValues());

  lcdShowPopup("Profile loaded!");
}

void lcdScalesTareTrigger(void) {
  LOG_VERBOSE("Tare scales");
  if (currentState.scalesPresent) currentState.tarePending = true;
}

void lcdHomeScreenScalesTrigger(void) {
  LOG_VERBOSE("Scales enabled or disabled");
  homeScreenScalesEnabled = lcdGetHomeScreenScalesEnabled();
}

void lcdBrewGraphScalesTareTrigger(void) {
  LOG_VERBOSE("Predictive scales tare action completed!");
  if (currentState.scalesPresent) {
    currentState.tarePending = true;
  }
  else {
    currentState.shotWeight = 0.f;
    predictiveWeight.setIsForceStarted(true);
  }
}

void lcdRefreshElementsTrigger(void) {

  eepromValues_t eepromCurrentValues = eepromGetCurrentValues();

  switch (lcdCurrentPageId) {
    case NextionPage::BrewPreinfusion:
      ACTIVE_PROFILE(eepromCurrentValues).preinfusionFlowState = lcdGetPreinfusionFlowState();
      break;
    case NextionPage::BrewProfiling:
      ACTIVE_PROFILE(eepromCurrentValues).mfProfileState = lcdGetProfileFlowState();
      break;
    case NextionPage::BrewTransitionProfile:
      ACTIVE_PROFILE(eepromCurrentValues).tpType = lcdGetTransitionFlowState();
      break;
    default:
      lcdShowPopup("Nope!");
      break;
  }

  // Make the necessary changes
  uploadPageCfg(eepromCurrentValues, systemState);
  // refresh the screen elements
  pageValuesRefresh();
}

void lcdQuickProfileSwitch(void) {
  lcdSwitchActiveToStoredProfile(eepromGetCurrentValues());
  lcdShowPopup("Profile switched!");
}

//#############################################################################################
//###############################____PROFILING_CONTROL____#####################################
//#############################################################################################
static void updateProfilerPhases(void) {
  float shotTarget = -1.f;

  if (ACTIVE_PROFILE(runningCfg).stopOnWeightState) {
    shotTarget = profileStopTarget(ACTIVE_PROFILE(runningCfg));
  }

  //update global stop conditions (currently only stopOnWeight is configured in nextion)
  profile.globalStopConditions = GlobalStopConditions{ .weight=shotTarget };

  profile.clear();

  //Setup release pressure + fill@7ml/sec
  if (runningCfg.basketPrefill) {
    addFillBasketPhase(7.f);
  }

  // Setup pre-infusion if needed
  if (ACTIVE_PROFILE(runningCfg).preinfusionState) {
    addPreinfusionPhases();
  }

  // Setup the soak phase if neecessary
  if (ACTIVE_PROFILE(runningCfg).soakState) {
    addSoakPhase();
  }
  preInfusionFinishedPhaseIdx = profile.phaseCount();

  addMainExtractionPhasesAndRamp();
}

void addPreinfusionPhases() {
  if (ACTIVE_PROFILE(runningCfg).preinfusionFlowState) { // flow based PI enabled
    float isPressureAbove = ACTIVE_PROFILE(runningCfg).preinfusionPressureAbove ? ACTIVE_PROFILE(runningCfg).preinfusionFlowPressureTarget : -1.f;
    float isWeightAbove = ACTIVE_PROFILE(runningCfg).preinfusionWeightAbove > 0.f ? ACTIVE_PROFILE(runningCfg).preinfusionWeightAbove : -1.f;
    float isWaterPumped = ACTIVE_PROFILE(runningCfg).preinfusionFilled > 0.f ? ACTIVE_PROFILE(runningCfg).preinfusionFilled : -1.f;

    addFlowPhase(Transition{ ACTIVE_PROFILE(runningCfg).preinfusionFlowVol }, ACTIVE_PROFILE(runningCfg).preinfusionFlowPressureTarget, ACTIVE_PROFILE(runningCfg).preinfusionFlowTime * 1000, isPressureAbove, -1, isWeightAbove, isWaterPumped);
  }
  else { // pressure based PI enabled
    // For now handling phase switching on restrictions here but as this grow will have to deal with it otherwise.
    float isPressureAbove = ACTIVE_PROFILE(runningCfg).preinfusionPressureAbove ? ACTIVE_PROFILE(runningCfg).preinfusionBar : -1.f;
    float isWeightAbove = ACTIVE_PROFILE(runningCfg).preinfusionWeightAbove > 0.f ? ACTIVE_PROFILE(runningCfg).preinfusionWeightAbove : -1.f;
    float isWaterPumped = ACTIVE_PROFILE(runningCfg).preinfusionFilled > 0.f ? ACTIVE_PROFILE(runningCfg).preinfusionFilled : -1.f;

    addPressurePhase(Transition{ ACTIVE_PROFILE(runningCfg).preinfusionBar }, ACTIVE_PROFILE(runningCfg).preinfusionPressureFlowTarget, ACTIVE_PROFILE(runningCfg).preinfusionSec * 1000, isPressureAbove, -1, isWeightAbove, isWaterPumped);
  }
}

void addSoakPhase() {
    uint16_t phaseSoak = ACTIVE_PROFILE(runningCfg).preinfusionFlowState ? ACTIVE_PROFILE(runningCfg).soakTimeFlow : ACTIVE_PROFILE(runningCfg).soakTimePressure;
    float maintainFlow = ACTIVE_PROFILE(runningCfg).soakKeepFlow > 0.f ? ACTIVE_PROFILE(runningCfg).soakKeepFlow : -1.f;
    float maintainPressure = ACTIVE_PROFILE(runningCfg).soakKeepPressure > 0.f ? ACTIVE_PROFILE(runningCfg).soakKeepPressure : -1.f;
    float isPressureBelow = ACTIVE_PROFILE(runningCfg).soakBelowPressure > 0.f ? ACTIVE_PROFILE(runningCfg).soakBelowPressure : -1.f;
    float isPressureAbove = ACTIVE_PROFILE(runningCfg).soakAbovePressure > 0.f ? ACTIVE_PROFILE(runningCfg).soakAbovePressure : -1.f;
    float isWeightAbove = ACTIVE_PROFILE(runningCfg).soakAboveWeight > 0.f ? ACTIVE_PROFILE(runningCfg).soakAboveWeight : -1.f;

    if (maintainPressure > 0.f)
      addPressurePhase(Transition{maintainPressure}, (maintainFlow > 0.f ? maintainFlow : 2.5f), phaseSoak * 1000, isPressureAbove, isPressureBelow, isWeightAbove, -1);
    else if(maintainFlow > 0.f)
      addFlowPhase(Transition{maintainFlow},  -1, phaseSoak * 1000, isPressureAbove, isPressureBelow, isWeightAbove, -1);
    else
      addPressurePhase(Transition{maintainPressure}, maintainFlow, phaseSoak * 1000, isPressureAbove, isPressureBelow, isWeightAbove, -1);
}

void addMainExtractionPhasesAndRamp() {
  int rampPhaseIndex = -1;

  if (ACTIVE_PROFILE(runningCfg).profilingState) {
    if (ACTIVE_PROFILE(runningCfg).tpState) {
      // ----------------- Transition Profile ----------------- //
      if (ACTIVE_PROFILE(runningCfg).tpType) { // flow based profiling enabled
        /* Setting the phase specific restrictions */
        /* ------------------------------------------ */
        float fpStart = ACTIVE_PROFILE(runningCfg).tfProfileStart;
        float fpEnd = ACTIVE_PROFILE(runningCfg).tfProfileEnd;
        uint32_t fpHold = ACTIVE_PROFILE(runningCfg).tfProfileHold * 1000UL;
        float holdLimit = ACTIVE_PROFILE(runningCfg).tfProfileHoldLimit > 0.f ? ACTIVE_PROFILE(runningCfg).tfProfileHoldLimit : -1;
        TransitionCurve curve = (TransitionCurve)ACTIVE_PROFILE(runningCfg).tfProfileSlopeShape;
        uint32_t curveTime = ACTIVE_PROFILE(runningCfg).tfProfileSlope * 1000UL;
        /* ------------------------------------------ */

        if (fpStart > 0.f && fpHold > 0) {
          addFlowPhase(Transition{ fpStart }, holdLimit, fpHold, -1, -1, -1, -1);
          rampPhaseIndex = rampPhaseIndex > 0 ? rampPhaseIndex : profile.phaseCount() - 1;
        }
        addFlowPhase(Transition{ fpStart, fpEnd, curve, curveTime }, ACTIVE_PROFILE(runningCfg).tfProfilingPressureRestriction, curveTime, -1, -1, -1, -1);
        rampPhaseIndex = rampPhaseIndex > 0 ? rampPhaseIndex : profile.phaseCount() - 1;
      }
      else { // pressure based profiling enabled
        /* Setting the phase specific restrictions */
        /* ------------------------------------------ */
        float ppStart = ACTIVE_PROFILE(runningCfg).tpProfilingStart;
        float ppEnd = ACTIVE_PROFILE(runningCfg).tpProfilingFinish;
        uint32_t ppHold = ACTIVE_PROFILE(runningCfg).tpProfilingHold * 1000UL;
        float holdLimit = ACTIVE_PROFILE(runningCfg).tpProfilingHoldLimit > 0.f ? ACTIVE_PROFILE(runningCfg).tpProfilingHoldLimit : -1;
        TransitionCurve curve = (TransitionCurve)ACTIVE_PROFILE(runningCfg).tpProfilingSlopeShape;
        uint32_t curveTime = ACTIVE_PROFILE(runningCfg).tpProfilingSlope * 1000UL;
        /* ------------------------------------------ */

        if (ppStart > 0.f && ppHold > 0) {
          addPressurePhase(Transition{ ppStart }, holdLimit, ppHold, -1, -1, -1, -1);
          rampPhaseIndex = rampPhaseIndex > 0 ? rampPhaseIndex : profile.phaseCount() - 1;
        }
        addPressurePhase(Transition{ ppStart, ppEnd, curve, curveTime }, ACTIVE_PROFILE(runningCfg).tpProfilingFlowRestriction, curveTime, -1, -1, -1, -1);
        rampPhaseIndex = rampPhaseIndex > 0 ? rampPhaseIndex : profile.phaseCount() - 1;
      }
    }

    // ----------------- Main Profile ----------------- //
    if (ACTIVE_PROFILE(runningCfg).mfProfileState) { // flow based profiling enabled
      /* Setting the phase specific restrictions */
      /* ------------------------------------------ */
      float fpStart = ACTIVE_PROFILE(runningCfg).mfProfileStart;
      float fpEnd = ACTIVE_PROFILE(runningCfg).mfProfileEnd;
      TransitionCurve curve = (TransitionCurve)ACTIVE_PROFILE(runningCfg).mfProfileSlopeShape;
      uint32_t curveTime = ACTIVE_PROFILE(runningCfg).mfProfileSlope * 1000UL;

      /* ------------------------------------------ */
      addFlowPhase(Transition(fpStart, fpEnd, curve, curveTime), ACTIVE_PROFILE(runningCfg).mfProfilingPressureRestriction, -1, -1, -1, -1, -1);
    }
    else { // pressure based profiling enabled
      /* Setting the phase specific restrictions */
      /* ------------------------------------------ */
      float ppStart = ACTIVE_PROFILE(runningCfg).mpProfilingStart;
      float ppEnd = ACTIVE_PROFILE(runningCfg).mpProfilingFinish;
      TransitionCurve curve = (TransitionCurve)ACTIVE_PROFILE(runningCfg).mpProfilingSlopeShape;
      uint32_t curveTime = ACTIVE_PROFILE(runningCfg).mpProfilingSlope * 1000UL;
      /* ------------------------------------------ */
      addPressurePhase(Transition(ppStart, ppEnd, curve, curveTime), ACTIVE_PROFILE(runningCfg).mpProfilingFlowRestriction, -1, -1, -1, -1, -1);
    }
  } else { // Shot profiling disabled. Default to 9 bars
    addPressurePhase(Transition(9.f), -1, -1, -1, -1, -1, -1);
  }

  rampPhaseIndex = rampPhaseIndex > 0 ? rampPhaseIndex : profile.phaseCount() - 1;
  insertRampPhaseIfNeeded(rampPhaseIndex);
}

// ------------ Insert a ramp phase in the rampPhaseIndex position ------------ //
void insertRampPhaseIfNeeded(size_t rampPhaseIndex) {
  uint16_t rampTime = ACTIVE_PROFILE(runningCfg).preinfusionRamp;
  TransitionCurve rampCurve = (TransitionCurve)ACTIVE_PROFILE(runningCfg).preinfusionRampSlope;

  if (rampPhaseIndex <= 0 || rampTime <= 0 || rampCurve == TransitionCurve::INSTANT) { // No ramp needed
    return;
  }

  // Get the phase currently in rampPhaseIndex - this is the phase we want to ramp to
  Phase targetPhase = profile.phases[rampPhaseIndex];
  float targetValue = targetPhase.target.isInstant() ? targetPhase.target.end : targetPhase.target.start;

  if (targetValue <= 0) { // No ramp needed, next phase will perform a ramp.
    return;
  }

  profile.insertPhase(Phase {
    .type           = targetPhase.type,
    .target         = Transition(targetValue, rampCurve, rampTime * 1000),
    .restriction    = -1,
    .stopConditions = PhaseStopConditions{ .time=rampTime * 1000 }
  }, rampPhaseIndex);
}

void addFillBasketPhase(float flowRate) {
  addFlowPhase(Transition(flowRate), -1, -1, 0.1f, -1, -1, -1);
}

void addPressurePhase(Transition pressure, float flowRestriction, int timeMs, float pressureAbove, float pressureBelow, float shotWeight, float isWaterPumped) {
  addPhase(PHASE_TYPE::PHASE_TYPE_PRESSURE, pressure, flowRestriction, timeMs, pressureAbove, pressureBelow, shotWeight, isWaterPumped);
}

void addFlowPhase(Transition flow, float pressureRestriction, int timeMs, float pressureAbove, float pressureBelow, float shotWeight, float isWaterPumped) {
  addPhase(PHASE_TYPE::PHASE_TYPE_FLOW, flow, pressureRestriction, timeMs, pressureAbove, pressureBelow, shotWeight, isWaterPumped);
}

void addPhase(PHASE_TYPE type, Transition target, float restriction, int timeMs, float pressureAbove, float pressureBelow, float shotWeight, float isWaterPumped) {
  profile.addPhase(Phase {
    .type           = type,
    .target         = target,
    .restriction    = restriction,
    .stopConditions = PhaseStopConditions{ .time=timeMs, .pressureAbove=pressureAbove, .pressureBelow=pressureBelow, .weight=shotWeight, .waterPumpedInPhase=isWaterPumped }
  });
}

void onProfileReceived(Profile& newProfile) {
}

static void profiling(void) {
  if (brewActive) { //runs this only when brew button activated and pressure profile selected
    uint32_t timeInShot = millis() - brewingTimer;
    phaseProfiler.updatePhase(timeInShot, currentState);
    CurrentPhase& currentPhase = phaseProfiler.getCurrentPhase();
    ShotSnapshot shotSnapshot = buildShotSnapshot(timeInShot, currentState, currentPhase);
    espCommsSendShotData(shotSnapshot, 100);

    if (phaseProfiler.isFinished()) {
      setPumpOff();
      closeValve();
      brewActive = false;
    } else if (currentPhase.getType() == PHASE_TYPE::PHASE_TYPE_PRESSURE) {
      float newBarValue = currentPhase.getTarget();
      float flowRestriction =  currentPhase.getRestriction();
      openValve();
      setPumpPressure(newBarValue, flowRestriction, currentState);
    } else {
      float newFlowValue = currentPhase.getTarget();
      float pressureRestriction =  currentPhase.getRestriction();
      openValve();
      setPumpFlow(newFlowValue, pressureRestriction, currentState);
    }
  } else {
    setPumpOff();
    closeValve();
  }
  // Keep that water at temp
  justDoCoffee(runningCfg, currentState, brewActive);
}

static void manualFlowControl(void) {
  if (brewActive) {
    openValve();
    float flow_reading = lcdGetManualFlowVol() / 10.f ;
    setPumpFlow(flow_reading, 0.f, currentState);
  } else {
    setPumpOff();
    closeValve();
  }
  justDoCoffee(runningCfg, currentState, brewActive);
}

//#############################################################################################
//###################################____BREW DETECT____#######################################
//#############################################################################################

static void brewDetect(void) {
  static bool paramsReset = true;
  // Do not allow brew detection while system reports not ready.
  if (!sysReadinessCheck()) {
    brewActive=false;paramsReset=true;
    // During a valid startup fill, fillBoiler owns the pump/valve. Do not
    // cancel that fill merely because normal brew readiness is still false.
    if(systemState.startupInitFinished||currentState.waterLvl<MIN_WATER_LVL){setPumpOff();closeValve();}
    return;
  }
  if (currentState.brewSwitchState) {
    if (!paramsReset) {
      lcdWakeUp();
      brewParamsReset();
      paramsReset = true;
      brewActive = true;
    }
    // needs to be here as it creates a locking state soemtimes if not kept up to date during brew
    // mainly when shotWeight restriction kick in.
    systemHealthTimer = millis() + HEALTHCHECK_EVERY;
  } else {
    brewActive = false;
    currentState.pumpClicks = getAndResetClickCounter();
    if (paramsReset) {
      brewParamsReset();
      paramsReset = false;
    }
  }
}

static void brewParamsReset(void) {
  currentState.tarePending = true;
  currentState.shotWeight  = 0.f;
  currentState.pumpFlow    = 0.f;
  currentState.weight      = 0.f;
  currentState.waterPumped = 0.f;
  brewingTimer             = millis();
  flowTimer                = brewingTimer;
  systemHealthTimer        = brewingTimer + HEALTHCHECK_EVERY;

  weightMeasurements.clear();
  predictiveWeight.reset();
  phaseProfiler.reset();
}

static bool sysReadinessCheck(void) {
  // Startup procedures not finished
  if (!systemState.startupInitFinished) {
    return false;
  }
  // If there's not enough water in the tank
  if (currentState.waterLvl < MIN_WATER_LVL)
  {
    lcdShowPopup("Fill the water tank!");
    return false;
  }

  return true;
}

static bool dripTrayAllowsPressureRelease(void) {
#if DRIP_TRAY_GUARD_ENABLED
  float grossWeight = NAN;
  const DripTrayStatus trayStatus = scalesGetDripTrayStatus(&grossWeight);

  switch (trayStatus) {
    case DripTrayStatus::Present:
      return true;
    case DripTrayStatus::Missing:
      LOG_ERROR("Pressure release blocked: drip tray gross weight is %.1lf g", static_cast<double>(grossWeight));
      lcdShowPopup("INSERT DRIP TRAY");
      return false;
    case DripTrayStatus::Unconfigured:
      LOG_ERROR("Pressure release blocked: drip tray guard is not calibrated");
      lcdShowPopup("TRAY GUARD SETUP");
      return false;
    case DripTrayStatus::Unavailable:
    default:
      LOG_ERROR("Pressure release blocked: hardware scales unavailable");
      lcdShowPopup("SCALES ERROR - NO RELEASE");
      return false;
  }
#else
  return true;
#endif
}

static inline void sysHealthCheck(float pressureThreshold) {
  //Reloading the watchdog timer, if this function fails to run MCU is rebooted
  watchdogReload();

  /* This *while* is here to prevent situations where the system failed to get a temp reading and temp reads as 0 or -7(cause of the offset)
  If we would use a non blocking function then the system would keep the SSR in HIGH mode which would most definitely cause boiler overheating */
  while (!std::isfinite(currentState.temperature) || currentState.temperature <= 0.0f || currentState.temperature >= 170.0f) {
    //Reloading the watchdog timer, if this function fails to run MCU is rebooted
    watchdogReload();
    /* In the event of the temp failing to read while the SSR is HIGH
    we force set it to LOW while trying to get a temp reading - IMPORTANT safety feature */
    setPumpOff();
    setBoilerOff();
    setSteamBoilerRelayOff();
    brewActive=false;homeFlush=false;abortCleaning();
    sensorReadSwitches();
    lcdListen();
    if(indigoActive())indigoUpdate();
    if (millis() > thermoTimer) {
      LOG_ERROR("Cannot read temp from thermocouple (last read: %.1lf)!", static_cast<double>(currentState.temperature));
      currentState.steamSwitchState ? lcdShowPopup("COOLDOWN") : lcdShowPopup("TEMP READ ERROR"); // writing a LCD message
      currentState.temperature  = thermocoupleRead() - runningCfg.offsetTemp;  // Making sure we're getting a value
      thermoTimer = millis() + GET_KTYPE_READ_EVERY;
    }
  }

  /*Shut down heaters if steam has been ON and unused fpr more than 10 minutes.*/
  while (currentState.isSteamForgottenON) {
    //Reloading the watchdog timer, if this function fails to run MCU is rebooted
    watchdogReload();
    lcdShowPopup("TURN STEAM OFF NOW!");
    setPumpOff();
    setBoilerOff();
    setSteamBoilerRelayOff();
    sensorReadSwitches();
    lcdListen();
    currentState.isSteamForgottenON = currentState.steamSwitchState;
    if(indigoActive())indigoUpdate();
  }

  //Releasing the excess pressure after steaming or brewing if necessary
  #if defined LEGO_VALVE_RELAY || defined SINGLE_BOARD

  // No point going through the whole thing if this first condition isn't met.
  if (homeFlush || currentState.brewSwitchState || currentState.steamSwitchState || currentState.hotWaterSwitchState) {
    systemHealthTimer = millis() + HEALTHCHECK_EVERY;
    return;
  }
  // Should enter the block every "systemHealthTimer" seconds
  if (millis() >= systemHealthTimer) {
    while (currentState.smoothedPressure >= pressureThreshold && currentState.temperature < 100.f
           && !currentState.brewSwitchState && !currentState.steamSwitchState && !currentState.hotWaterSwitchState)
    {
      //Reloading the watchdog timer, if this function fails to run MCU is rebooted
      watchdogReload();
      switch (lcdCurrentPageId) {
        case NextionPage::BrewManual:
        case NextionPage::BrewGraph:
        case NextionPage::GraphPreview:
          brewDetect();
          lcdRefresh();
          lcdListen();
          sensorsRead();
          justDoCoffee(runningCfg, currentState, brewActive);
          break;
        default:
          sensorsRead();
          setPumpOff();
          setBoilerOff();
          setSteamValveRelayOff();
          setSteamBoilerRelayOff();
          if (!dripTrayAllowsPressureRelease()) {
            closeValve();
            systemHealthTimer = millis() + DRIP_TRAY_RETRY_MS;
            return;
          }
          lcdShowPopup("Releasing pressure!");
          if(indigoActive())indigoUpdate();
          openValve();
          break;
      }
    }
    closeValve();
    systemHealthTimer = millis() + HEALTHCHECK_EVERY;
  }
  // Throwing a pressure release countodown.
  if (lcdCurrentPageId == NextionPage::BrewGraph) return;
  if (lcdCurrentPageId == NextionPage::BrewManual) return;

  if (currentState.smoothedPressure >= pressureThreshold && currentState.temperature < 100.f) {
    if (millis() >= systemHealthTimer - 3500ul && millis() <= systemHealthTimer - 500ul) {
      char tmp[25];
      int countdown = (int)(systemHealthTimer-millis())/1000;
      unsigned int check = snprintf(tmp, sizeof(tmp), "Dropping beats in: %i", countdown);
      if (check > 0 && check <= sizeof(tmp)) {
        lcdShowPopup(tmp);
      }
    }
  }
  #endif
}

// Function to track time since system has started
static unsigned long getTimeSinceInit(void) {
  static unsigned long startTime = millis();
  return millis() - startTime;
}

static void fillBoiler(void) {
#if defined LEGO_VALVE_RELAY || defined SINGLE_BOARD
  static StartupFillClock fillClock;
  static DripTrayStatus notified = DripTrayStatus::Present;
  if (systemState.startupInitFinished) return;
  // A user-controlled flush owns outputs independently of automatic fill.
  if (homeFlush) {fillClock.reset();return;}
  if(currentState.waterLvl<MIN_WATER_LVL||isSwitchOn()){
    fillClock.reset();setPumpOff();closeValve();setBoilerOff();
    return;
  }
  if (currentState.temperature > BOILER_FILL_SKIP_TEMP) {
    setPumpOff();closeValve();systemState.startupInitFinished = true;return;
  }
  const auto tray = scalesGetDripTrayStatus();
  if (tray != DripTrayStatus::Present) {
    fillClock.reset();setPumpOff();closeValve();setBoilerOff();
    if (tray != notified) {
      notified = tray;
      lcdShowPopup(tray == DripTrayStatus::Unconfigured ? "Set up Tray guard in Menu before boiler fill" :
                   tray == DripTrayStatus::Missing ? "Place drip tray before boiler fill" :
                   "Need fresh hardware scales before boiler fill");
    }
    return;
  }
  notified = DripTrayStatus::Present;
  const uint32_t elapsed = fillClock.elapsed(millis());
  if (isBoilerFillPhase(elapsed)) fillBoilerUntilThreshod(elapsed);
#else
  systemState.startupInitFinished = true;
#endif
}

static bool isBoilerFillPhase(unsigned long elapsedTime) {
  return (indigoActive() || lcdCurrentPageId == NextionPage::Home) && elapsedTime >= BOILER_FILL_START_TIME;
}

static bool isBoilerFull(unsigned long elapsedTime) {
  bool boilerFull = false;
  if (elapsedTime > BOILER_FILL_START_TIME + 1000UL) {
    boilerFull =  (previousSmoothedPressure - currentState.smoothedPressure > -0.02f)
                &&
                  (previousSmoothedPressure - currentState.smoothedPressure < 0.001f);
  }

  return elapsedTime >= BOILER_FILL_TIMEOUT || boilerFull;
}

// Checks if Brew switch is ON
static bool isSwitchOn(void) {
  return indigoActive() ? (currentState.brewSwitchState || currentState.steamSwitchState || currentState.hotWaterSwitchState)
                        : currentState.brewSwitchState && lcdCurrentPageId == NextionPage::Home;
}

static void fillBoilerUntilThreshod(unsigned long elapsedTime) {
  if (elapsedTime >= BOILER_FILL_TIMEOUT) {
    setPumpOff();closeValve();
    systemState.startupInitFinished = true;
    return;
  }

  if (isBoilerFull(elapsedTime)) {
    closeValve();
    setPumpOff();
    systemState.startupInitFinished = true;
    return;
  }

  lcdShowPopup("Filling boiler!");
  openValve();
  setPumpToRawValue(35);
}

static void updateStartupTimer(void) {
  static uint32_t last=0xffffffffu;
  const auto seconds=getTimeSinceInit()/1000;
  if(seconds!=last){lcdSetUpTime(seconds);last=seconds;}
}

static void cpsInit(eepromValues_t &eepromValues) {
  int cps = getCPS();
  if (cps > 110) { // double 60 Hz
    eepromValues.powerLineFrequency = 60u;
  } else if (cps > 80) { // double 50 Hz
    eepromValues.powerLineFrequency = 50u;
  } else if (cps > 55) { // 60 Hz
    eepromValues.powerLineFrequency = 60u;
  } else if (cps > 0) { // 50 Hz
    eepromValues.powerLineFrequency = 50u;
  }
}

static void doLed(void) {
  if (!runningCfg.ledState) {led.setColor(0,0,0);return;}
  if (runningCfg.ledDisco && brewActive) {
    switch(lcdCurrentPageId) {
      case NextionPage::BrewGraph:
      case NextionPage::BrewManual:
        led.setDisco(led.CLASSIC);
        break;
      case NextionPage::Flush:
        led.setDisco(led.STROBE);
        break;
      case NextionPage::Descale:
        led.setDisco(led.DESCALE);
        break;
      default:
        led.setColor(0, 0, 0);
        break;
    }
  } else {
    switch(lcdCurrentPageId) {
      case NextionPage::Led:
        static uint32_t timer = millis();
        if (millis() > timer) {
          timer = millis() + 100u;
          lcdFetchLed(runningCfg);
        }
      default: // intentionally fall through
        led.setColor(runningCfg.ledR, runningCfg.ledG, runningCfg.ledB);
    }
  }
}


static bool indigoCommand(Indigo::Command command,int index){
  using Indigo::Command;
  const bool busy=currentState.brewSwitchState||brewActive||currentState.steamSwitchState||currentState.hotWaterSwitchState||homeFlush;
  auto deny=[](const char* why){lcdShowPopup(why);return false;};
  switch(command){
   case Command::Tare: if(index==1)lcdBrewGraphScalesTareTrigger();else lcdScalesTareTrigger();return true;
   case Command::Flush:
    if(homeFlush){homeFlush=false;flushDeactivated();return true;}
    if(busy||cleaningMode)return deny("Stop other modes before flushing");
    if(currentState.waterLvl<10)return deny("Fill the water tank");
    if(static_cast<uint32_t>(millis()-indigoUi()->lastContact())>=1500)return deny("Waiting for display connection");
    homeFlush=true;return true;
   case Command::SelectProfile:{
    if(busy||index<0||index>=5)return deny("Stop active mode before selecting");
    if(runningCfg.activeProfile==index)return true;
    auto saved=eepromGetCurrentValues();saved.activeProfile=index;
    if(!eepromWrite(saved))return deny("Profile selection not saved");
    runningCfg.activeProfile=index;updateProfilerPhases();lcdUploadProfile(runningCfg);return true;
   }
   case Command::SaveProfile:{
    if(busy||index<0||index>=5)return deny("Stop active mode before saving");
    auto saved=profileSaveValues(eepromGetCurrentValues(),runningCfg,index);
    if(!eepromWrite(saved))return deny("Profile values out of range");
    watchdogReload();lcdShowPopup("Profile saved");return true;
   }
   case Command::SaveSystem:{
    if(busy)return deny("Stop active mode before saving");
    auto merged=systemSaveValues(eepromGetCurrentValues(),runningCfg);
    if(!eepromWrite(merged))return deny("Settings out of range");
    watchdogReload();lcdShowPopup("System settings saved");return true;
   }
   case Command::CaptureFull:
    if(busy||!tof.valid()||tof.distance()<1||tof.distance()>=125)return deny("Need a fresh valid full-tank range");
    runningCfg.tankFullDistanceMm=tof.distance();lcdShowPopup("Full point captured - Save to retain");return true;
   case Command::CaptureTray:
    if(busy||!scalesCaptureTrayZero(runningCfg))return deny("Need idle, fresh and settled hardware scales");
    lcdShowPopup("No-tray zero captured. Refit tray, then Save");return true;
   case Command::SettingsChanged:
    if(busy)return false;
    scalesConfigure(runningCfg);pumpInit(runningCfg.powerLineFrequency,runningCfg.pumpFlowAtZero);
    if(index<0||index==runningCfg.activeProfile)updateProfilerPhases();return true;
   case Command::DefaultProfile:
    if(busy||index<0||index>=5)return deny("Stop active mode before loading defaults");
    runningCfg.profiles[index]=eepromGetDefaultValues().profiles[index];if(index==runningCfg.activeProfile)updateProfilerPhases();return true;
   case Command::ResetAll:
    if(busy)return deny("Stop active mode before resetting");
    runningCfg=eepromGetDefaultValues();scalesConfigure(runningCfg);pumpInit(runningCfg.powerLineFrequency,runningCfg.pumpFlowAtZero);updateProfilerPhases();if(!eepromWrite(runningCfg))return deny("Reset failed");watchdogReload();lcdShowPopup("Settings reset");return true;
   case Command::EnterManual:
    if(busy||cleaningMode)return deny("Stop active mode before Manual");manualMode=true;return true;
   case Command::ExitManual:
    if(currentState.brewSwitchState||brewActive)return deny("Turn brew switch OFF before leaving Manual");manualMode=false;return true;
   case Command::EnterBackflush:case Command::EnterDescale:
    if(busy)return deny("Turn physical switches OFF first");manualMode=false;cleaningMode=command==Command::EnterBackflush?1:2;return true;
   case Command::ExitCleaning:
    if(currentState.brewSwitchState)return deny("Turn brew switch OFF to leave cleaning");cleaningMode=0;flushDeactivated();return true;
   case Command::Preview:{if(busy)return deny("Stop active mode before preview");auto copy=runningCfg;copy.activeProfile=index;lcdUploadProfile(copy);return true;}
   case Command::Graph:return true;
  }
  return false;
}
static void indigoUpdate(){
  Indigo::Telemetry t;
  t.physicalBrew=currentState.brewSwitchState;t.brewing=brewActive&&!cleaningMode;t.steam=currentState.steamSwitchState;t.hotWater=currentState.hotWaterSwitchState;t.flushing=homeFlush;t.startup=systemState.startupInitFinished;
  t.waterValid=tof.valid();t.distance=tof.distance();static float lastElapsed=0;if(brewActive)lastElapsed=(millis()-brewingTimer)/1000.f;t.elapsed=lastElapsed;
  t.trayStatus=static_cast<uint8_t>(scalesGetDripTrayStatus(&t.gross));t.cleaningCycle=lcdGetDescaleCycle();t.uptime=getTimeSinceInit()/1000;
  indigoTick(t);
}
