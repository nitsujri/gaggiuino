#include <cmath>
#include <unity.h>

#include "boiler_fill.h"

namespace {
void expectFillState(BoilerFillState expected, BoilerFillState actual) {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(expected), static_cast<int>(actual));
}

// Existing water/containers are deliberately part of the starting load.
void startFill(BoilerFill &fill, uint32_t start = 100, float weight = 500.f) {
  for (uint32_t elapsed = 0; elapsed < BoilerFill::START_DELAY_MS; elapsed += 100)
    expectFillState(BoilerFillState::Waiting, fill.update(weight, start + elapsed, start + elapsed));
  const auto time = start + BoilerFill::START_DELAY_MS;
  expectFillState(BoilerFillState::Filling, fill.update(weight, time, time));
}
}

void test_boiler_fill_requires_new_water_for_a_full_second(void) {
  BoilerFill fill;
  startFill(fill);
  for (uint32_t t = 3200; t <= 5000; t += 100)
    expectFillState(BoilerFillState::Filling, fill.update(500.f, t, t));
  for (uint32_t t = 5100; t < 6100; t += 100)
    expectFillState(BoilerFillState::Filling, fill.update(503.f, t, t));
  expectFillState(BoilerFillState::Complete, fill.update(503.f, 6100, 6100));
}

void test_boiler_fill_dips_and_spikes_restart_confirmation(void) {
  BoilerFill fill;
  startFill(fill);
  for (uint32_t t = 3200; t <= 5000; t += 100)
    expectFillState(BoilerFillState::Filling, fill.update(t % 1000 == 0 ? 502.9f : 504.f, t, t));
  for (uint32_t t = 5100; t < 6100; t += 100)
    expectFillState(BoilerFillState::Filling, fill.update(503.f, t, t));
  expectFillState(BoilerFillState::Complete, fill.update(503.f, 6100, 6100));
}

void test_boiler_fill_runs_pump_for_at_least_two_seconds(void) {
  BoilerFill fill;
  startFill(fill);
  for (uint32_t t = 3200; t < 5100; t += 100)
    expectFillState(BoilerFillState::Filling, fill.update(503.f, t, t));
  expectFillState(BoilerFillState::Complete, fill.update(503.f, 5100, 5100));
}

void test_boiler_fill_waits_for_a_settled_baseline(void) {
  BoilerFill fill;
  for (uint32_t t = 100; t <= 3500; t += 100)
    expectFillState(BoilerFillState::Waiting, fill.update(t % 200 ? 500.f : 501.f, t, t));
  for (uint32_t t = 3600; t < 4500; t += 100)
    expectFillState(BoilerFillState::Waiting, fill.update(500.f, t, t));
  expectFillState(BoilerFillState::Filling, fill.update(500.f, 4500, 4500));
}

void test_boiler_fill_uses_high_end_of_baseline(void) {
  BoilerFill fill;
  for (uint32_t t = 100; t <= 3100; t += 100)
    fill.update(t % 200 ? 500.f : 500.5f, t, t);
  for (uint32_t t = 3200; t <= 5500; t += 100)
    expectFillState(BoilerFillState::Filling, fill.update(503.f, t, t));
  for (uint32_t t = 5600; t < 6600; t += 100)
    expectFillState(BoilerFillState::Filling, fill.update(503.5f, t, t));
  expectFillState(BoilerFillState::Complete, fill.update(503.5f, 6600, 6600));
}

void test_boiler_fill_cached_or_missing_conversions_cannot_complete(void) {
  BoilerFill fill;
  startFill(fill);
  expectFillState(BoilerFillState::Filling, fill.update(503.f, 3200, 3200));
  expectFillState(BoilerFillState::Filling, fill.update(503.f, 3200, 3500));
  expectFillState(BoilerFillState::Waiting, fill.update(503.f, 3200, 3501));
  // A gap between fresh samples also invalidates the old baseline/confirmation.
  fill.reset();
  startFill(fill);
  expectFillState(BoilerFillState::Filling, fill.update(503.f, 3200, 3200));
  expectFillState(BoilerFillState::Waiting, fill.update(503.f, 4200, 4200));
}

void test_boiler_fill_rejects_invalid_weights(void) {
  const float invalidWeights[] = {NAN, INFINITY, -INFINITY};
  for (float invalid : invalidWeights) {
    BoilerFill fill;
    startFill(fill);
    expectFillState(BoilerFillState::Waiting, fill.update(invalid, 3200, 3200));
    expectFillState(BoilerFillState::Waiting, fill.update(503.f, 3300, 3300));
  }
}

void test_boiler_fill_waits_beyond_old_timeout_but_latches_at_pump_limit(void) {
  BoilerFill fill;
  startFill(fill);
  for (uint32_t t = 3200; t < 33100; t += 100)
    expectFillState(BoilerFillState::Filling, fill.update(500.f, t, t));
  expectFillState(BoilerFillState::TimedOut, fill.update(500.f, 33100, 33100));
  expectFillState(BoilerFillState::TimedOut, fill.update(510.f, 40000, 40000));
}

void test_boiler_fill_interruption_requires_new_baseline_and_wait(void) {
  BoilerFill fill;
  startFill(fill);
  fill.reset();
  startFill(fill, 60000, 530.f);
  for (uint32_t t = 63100; t <= 65000; t += 100)
    expectFillState(BoilerFillState::Filling, fill.update(530.f, t, t));
}

void test_boiler_fill_timers_handle_millis_wrap(void) {
  BoilerFill fill;
  const uint32_t start = UINT32_MAX - 3999;
  startFill(fill, start);
  for (uint32_t elapsed = 3100; elapsed < 5000; elapsed += 100) {
    const uint32_t time = start + elapsed;
    expectFillState(BoilerFillState::Filling, fill.update(503.f, time, time));
  }
  const uint32_t time = start + 5000;
  expectFillState(BoilerFillState::Complete, fill.update(503.f, time, time));
}

void runAllBoilerFillTests(void) {
  RUN_TEST(test_boiler_fill_requires_new_water_for_a_full_second);
  RUN_TEST(test_boiler_fill_dips_and_spikes_restart_confirmation);
  RUN_TEST(test_boiler_fill_runs_pump_for_at_least_two_seconds);
  RUN_TEST(test_boiler_fill_waits_for_a_settled_baseline);
  RUN_TEST(test_boiler_fill_uses_high_end_of_baseline);
  RUN_TEST(test_boiler_fill_cached_or_missing_conversions_cannot_complete);
  RUN_TEST(test_boiler_fill_rejects_invalid_weights);
  RUN_TEST(test_boiler_fill_waits_beyond_old_timeout_but_latches_at_pump_limit);
  RUN_TEST(test_boiler_fill_interruption_requires_new_baseline_and_wait);
  RUN_TEST(test_boiler_fill_timers_handle_millis_wrap);
}

#ifdef STANDALONE_BOILER_FILL_TEST
void setUp() {}
void tearDown() {}
int main() {
  UNITY_BEGIN();
  runAllBoilerFillTests();
  return UNITY_END();
}
#endif
