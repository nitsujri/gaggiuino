#include "descale.h"
#include "../lcd/lcd.h"
#include "../peripherals/scales.h"
#include "just_do_coffee.h"

DescalingState descalingState = DescalingState::IDLE;
short flushCounter = 0;
namespace {
uint32_t descaleTimer = 0, pulseTimer = 0, backStart = 0, backPhase = 0;
unsigned cycle = 0, pulse = 0;
bool beating = false, backStarted = false, backAborted = false;
bool guard(const SensorState &s) {
  if (s.waterLvl < 10 || s.steamSwitchState || s.hotWaterSwitchState)
    return false;
#if DRIP_TRAY_GUARD_ENABLED
  return scalesGetDripTrayStatus() == DripTrayStatus::Present;
#else
  return true;
#endif
}
void stopped() {
  flushDeactivated();
  setSteamValveRelayOff();
  setBoilerOff();
  setSteamBoilerRelayOff();
}
void finish() {
  stopped();
  descalingState = DescalingState::FINISHED;
  beating = false;
}
} // namespace
void abortCleaning() {
  finish();
  backAborted = true;
}
void deScale(eepromValues_t &cfg, const SensorState &s) {
  (void)cfg;
  // Descaling is unheated. Never write a temporary temperature into a recipe.
  setBoilerOff();
  setSteamBoilerRelayOff();
  if (!s.brewSwitchState) {
    finish();
    descalingState = DescalingState::IDLE;
    cycle = 0;
    return;
  }
  if (descalingState == DescalingState::FINISHED) {
    stopped();
    return;
  }
  if (!guard(s)) {
    finish();
    lcdShowPopup("Cleaning stopped: check tank / tray");
    return;
  }
  if (descalingState == DescalingState::IDLE) {
    openValve();
    setSteamValveRelayOn();
    descalingState = DescalingState::DESCALING_PHASE1;
    descaleTimer = millis();
    cycle = 0;
  }
  const auto now = millis();
  if (beating) {
    // Original 1s closed / 200ms open, three closures. Each loop is interruptible.
    const uint32_t duration = pulse % 2 ? 200 : 1000;
    if (static_cast<uint32_t>(now - pulseTimer) >= duration) {
      pulseTimer = now;
      pulse++;
      if (pulse >= 5) {
        openValve();
        setPumpOff();
        beating = false;
        lcdSetDescaleCycle(cycle++);
        if (cycle >= 100)
          finish();
        else {
          descalingState = DescalingState::DESCALING_PHASE1;
          descaleTimer = now;
        }
        return;
      }
    }
    setPumpFullOn();
    if (pulse % 2)
      openValve();
    else
      closeValve();
    return;
  }
  switch (descalingState) {
  case DescalingState::DESCALING_PHASE1:
    setPumpToRawValue(10);
    if (static_cast<uint32_t>(now - descaleTimer) > DESCALE_PHASE1_EVERY) {
      lcdSetDescaleCycle(cycle++);
      if (cycle >= 100)
        finish();
      else {
        descaleTimer = now;
        descalingState = DescalingState::DESCALING_PHASE2;
      }
    }
    break;
  case DescalingState::DESCALING_PHASE2:
    setPumpOff();
    if (static_cast<uint32_t>(now - descaleTimer) > DESCALE_PHASE2_EVERY) {
      lcdSetDescaleCycle(cycle++);
      descaleTimer = now;
      descalingState = DescalingState::DESCALING_PHASE3;
    }
    break;
  case DescalingState::DESCALING_PHASE3:
    setPumpToRawValue(30);
    if (static_cast<uint32_t>(now - descaleTimer) > DESCALE_PHASE3_EVERY) {
      beating = true;
      pulse = 0;
      pulseTimer = now;
      closeValve();
      setPumpFullOn();
    }
    break;
  default:
    break;
  }
}
void backFlush(const SensorState &s) {
  const auto now = millis();
  if (!s.brewSwitchState) {
    flushDeactivated();
    flushCounter = 0;
    backStart = now;
    backPhase = now;
    backStarted = false;
    backAborted = false;
    return;
  }
  if (backAborted || flushCounter >= 11) {
    flushDeactivated();
    return;
  }
  if (!guard(s)) {
    backAborted = true;
    flushDeactivated();
    lcdShowPopup("Flush stopped: check tank / tray");
    return;
  }
  if (!backStarted) {
    if (static_cast<uint32_t>(now - backStart) > 7000 && s.smoothedPressure > 5.f) {
      backStarted = true;
      backPhase = now;
    } else {
      flushActivated();
      return;
    }
  }
  flushPhases();
}
void flushActivated() {
#if defined(SINGLE_BOARD) || defined(LEGO_VALVE_RELAY)
  openValve();
#endif
  setPumpFullOn();
}
void flushDeactivated() {
#if defined(SINGLE_BOARD) || defined(LEGO_VALVE_RELAY)
  closeValve();
#endif
  setPumpOff();
}
void flushPhases() {
  if (static_cast<uint32_t>(millis() - backPhase) >= 5000) {
    flushCounter++;
    backPhase = millis();
  }
  if (flushCounter >= 11)
    flushDeactivated();
  else if (flushCounter % 2)
    flushActivated();
  else
    flushDeactivated();
  lcdSetDescaleCycle(flushCounter);
}
