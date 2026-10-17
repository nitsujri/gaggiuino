// Exercise the production cleaning implementation with mocked physical inputs.
#define DRIP_TRAY_GUARD_ENABLED 1
#include "../../src/functional/descale.cpp"
#include "indigo/control.h"
#include <ArduinoFake.h>
using namespace fakeit;
namespace CleaningTest {
uint32_t now = 0;
DripTrayStatus tray = DripTrayStatus::Present;
void setup() {
  ArduinoFakeReset();
  When(Method(ArduinoFake(), millis)).AlwaysDo([]() -> unsigned long {
    return now;
  });
  When(Method(ArduinoFake(), digitalWrite)).AlwaysReturn();
  When(Method(ArduinoFake(), delay)).AlwaysReturn();
  tray = DripTrayStatus::Present;
  now = 0;
  lastPumpValue = 0;
}
void backflush_has_pressure_gate_and_fail_closed_latch() {
  setup();
  SensorState s{};
  s.waterLvl = 100;
  backFlush(s);
  s.brewSwitchState = true;
  backFlush(s);
  TEST_ASSERT_EQUAL(100, lastPumpValue);
  now = 7001;
  s.smoothedPressure = 6;
  backFlush(s);
  TEST_ASSERT_EQUAL(0, lastPumpValue);
  now = 12002;
  backFlush(s);
  TEST_ASSERT_EQUAL(100, lastPumpValue);
  tray = DripTrayStatus::Missing;
  backFlush(s);
  TEST_ASSERT_EQUAL(0, lastPumpValue);
  tray = DripTrayStatus::Present;
  backFlush(s);
  TEST_ASSERT_EQUAL(0, lastPumpValue);
  s.brewSwitchState = false;
  backFlush(s);
  s.brewSwitchState = true;
  backFlush(s);
  TEST_ASSERT_EQUAL(100, lastPumpValue);
  s.brewSwitchState = false;
  backFlush(s);
}
void descale_is_interruptible_during_valve_pulses() {
  setup();
  eepromValues_t c{};
  c.profiles[0].setpoint = 92;
  SensorState s{};
  s.waterLvl = 100;
  deScale(c, s);
  s.brewSwitchState = true;
  deScale(c, s);
  TEST_ASSERT_EQUAL(10, lastPumpValue);
  now = 30001;
  deScale(c, s);
  deScale(c, s);
  TEST_ASSERT_EQUAL(0, lastPumpValue);
  now = 90002;
  deScale(c, s);
  deScale(c, s);
  TEST_ASSERT_EQUAL(30, lastPumpValue);
  now = 100003;
  deScale(c, s);
  TEST_ASSERT_EQUAL(100, lastPumpValue);
  tray = DripTrayStatus::Unavailable;
  deScale(c, s);
  TEST_ASSERT_EQUAL(0, lastPumpValue);
  TEST_ASSERT_EQUAL(static_cast<int>(DescalingState::FINISHED),
                    static_cast<int>(descalingState));
  tray = DripTrayStatus::Present;
  deScale(c, s);
  TEST_ASSERT_EQUAL(0, lastPumpValue);
  s.brewSwitchState = false;
  deScale(c, s);
  TEST_ASSERT_EQUAL(92, c.profiles[0].setpoint);
  Verify(Method(ArduinoFake(), delay)).Never();
}
void cleaning_rejects_unconfigured_guard_and_low_water() {
  setup();
  SensorState s{};
  eepromValues_t c{};
  s.waterLvl = 100;
  backFlush(s);
  deScale(c, s);
  s.brewSwitchState = true;
  tray = DripTrayStatus::Unconfigured;
  backFlush(s);
  deScale(c, s);
  TEST_ASSERT_EQUAL(0, lastPumpValue);
  s.brewSwitchState = false;
  backFlush(s);
  deScale(c, s);
  tray = DripTrayStatus::Present;
  s.waterLvl = 9;
  s.brewSwitchState = true;
  backFlush(s);
  deScale(c, s);
  TEST_ASSERT_EQUAL(0, lastPumpValue);
  s.brewSwitchState = false;
  backFlush(s);
  deScale(c, s);
}
void home_flush_gates_all_switches_and_display_age() {
  SensorState s{};
  s.waterLvl = 100;
  for (unsigned bits = 0; bits < 8; bits++)
    for (unsigned age : {1499u, 1500u}) {
      s.brewSwitchState = bits & 1;
      s.steamSwitchState = bits & 2;
      s.hotWaterSwitchState = bits & 4;
      TEST_ASSERT_EQUAL(bits == 0 && age < 1500, homeFlushAllowed(s, age));
    }
  s = {};
  s.waterLvl = 9;
  TEST_ASSERT_FALSE(homeFlushAllowed(s, 0));
  s.waterLvl = 100;
  // Actual gross/status has no role in the explicitly manual Home Flush.
  for (auto status : {DripTrayStatus::Missing, DripTrayStatus::Unavailable,
                      DripTrayStatus::Unconfigured}) {
    tray = status;
    TEST_ASSERT_TRUE(homeFlushAllowed(s, 0));
  }
}
void startup_fill_clock_restarts_after_waiting_for_tray() {
  StartupFillClock clock;
  TEST_ASSERT_EQUAL(0, clock.elapsed(100000));
  TEST_ASSERT_EQUAL(3000, clock.elapsed(103000));
  clock.reset(); // Missing tray pauses pump and discards elapsed fill budget.
  TEST_ASSERT_EQUAL(0, clock.elapsed(1000000));
  TEST_ASSERT_EQUAL(8000, clock.elapsed(1008000));
  clock.reset();
  TEST_ASSERT_EQUAL(0, clock.elapsed(0xfffffff0u));
  TEST_ASSERT_EQUAL(32, clock.elapsed(16));
}
void provisional_tray_threshold_is_150g_and_fails_closed() {
  TEST_ASSERT_EQUAL(
      static_cast<int>(DripTrayStatus::Missing),
      static_cast<int>(classifyDripTray(true, true, true, 149.9f, 150.f)));
  TEST_ASSERT_EQUAL(
      static_cast<int>(DripTrayStatus::Present),
      static_cast<int>(classifyDripTray(true, true, true, 150.f, 150.f)));
  TEST_ASSERT_EQUAL(
      static_cast<int>(DripTrayStatus::Unconfigured),
      static_cast<int>(classifyDripTray(false, true, true, 500.f, 150.f)));
  TEST_ASSERT_EQUAL(
      static_cast<int>(DripTrayStatus::Unavailable),
      static_cast<int>(classifyDripTray(true, false, true, 500.f, 150.f)));
}

void sample_freshness_handles_clock_wrap_and_missing_conversion() {
  const long okay[2] = {100000, -200000}, flat[2] = {0, 100000},
             rail[2] = {8388607, 100000};
  TEST_ASSERT_TRUE(hx711RawPairValid(okay));
  TEST_ASSERT_FALSE(hx711RawPairValid(flat));
  TEST_ASSERT_FALSE(hx711RawPairValid(rail));
  TEST_ASSERT_FALSE(sampleIsFresh(false, 100, 100, 300));
  TEST_ASSERT_TRUE(sampleIsFresh(true, 100, 400, 300));
  TEST_ASSERT_FALSE(sampleIsFresh(true, 100, 401, 300));
  TEST_ASSERT_TRUE(sampleIsFresh(true, 0xfffffff0u, 10, 300));
  TEST_ASSERT_FALSE(sampleIsFresh(true, 0xfffffff0u, 400, 300));
}
void run() {
  RUN_TEST(backflush_has_pressure_gate_and_fail_closed_latch);
  RUN_TEST(descale_is_interruptible_during_valve_pulses);
  RUN_TEST(cleaning_rejects_unconfigured_guard_and_low_water);
  RUN_TEST(home_flush_gates_all_switches_and_display_age);
  RUN_TEST(startup_fill_clock_restarts_after_waiting_for_tray);
  RUN_TEST(provisional_tray_threshold_is_150g_and_fails_closed);
  RUN_TEST(sample_freshness_handles_clock_wrap_and_missing_conversion);
  ArduinoFakeReset();
}
} // namespace CleaningTest
DripTrayStatus scalesGetDripTrayStatus(float *g) {
  if (g)
    *g = 170;
  return CleaningTest::tray;
}
void lcdShowPopup(const char *) {}
void lcdSetDescaleCycle(int) {}
