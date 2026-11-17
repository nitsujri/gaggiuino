#include <cmath>
#include <unity.h>
#include "shot_stop.h"
#include "profiling_phases.h"

namespace {
ScaleControlSample shotSample(float weight, uint32_t time, uint32_t delay = 50) {
  ScaleControlSample sample;
  sample.weightG = weight;
  sample.sampledMillis = time;
  sample.smoothingDelayMs = delay;
  sample.valid = true;
  return sample;
}

void seedShotFlow(ShotStopPredictor &predictor, float rate = 6.f, uint32_t start = 800) {
  predictor.reset(start);
  for (unsigned i = 0; i < 4; ++i) {
    const uint32_t time = start + i * 100;
    TEST_ASSERT_TRUE(predictor.add(shotSample(31.2f + rate * i * 0.1f, time), time));
  }
}
}

void test_shot_stop_projects_mass_age_filter_delay_and_inflight_tail(void) {
  ShotStopPredictor predictor;
  seedShotFlow(predictor); // latest filtered weight 33 g at 1100 ms, centered at 1050 ms
  const auto prediction = predictor.predict(1200, 500);
  TEST_ASSERT_TRUE(prediction.active && prediction.flowReady && !prediction.expired);
  TEST_ASSERT_FLOAT_WITHIN(0.0001, 6.f, prediction.flowGPerS);
  TEST_ASSERT_FLOAT_WITHIN(0.0001, 33.f, prediction.measuredWeightG);
  TEST_ASSERT_FLOAT_WITHIN(0.0001, 33.9f, prediction.currentWeightG);
  TEST_ASSERT_FLOAT_WITHIN(0.0001, 36.9f, prediction.finalWeightG);
  TEST_ASSERT_EQUAL_UINT32(100, prediction.sampleAgeMs);
  TEST_ASSERT_EQUAL_UINT32(50, prediction.smoothingDelayMs);
  TEST_ASSERT_TRUE(prediction.shouldStop(36.8f));
  TEST_ASSERT_FALSE(prediction.shouldStop(37.f));
  TEST_ASSERT_FLOAT_WITHIN(0.0001, 33.9f, predictor.predict(1200, 0).finalWeightG);
}

void test_shot_stop_bridges_rejected_samples_without_zeroing_flow(void) {
  ShotStopPredictor predictor;
  seedShotFlow(predictor);
  const auto cached = shotSample(33.f, 1100);
  for (uint32_t now = 1101; now <= 1500; ++now) TEST_ASSERT_FALSE(predictor.add(cached, now));
  TEST_ASSERT_FLOAT_WITHIN(0.0001, 6.f, predictor.predict(1450, 500).flowGPerS);
  TEST_ASSERT_FALSE(predictor.predict(1250, 500).shouldStop(37.49f));
  TEST_ASSERT_TRUE(predictor.predict(1300, 500).shouldStop(37.49f));
  TEST_ASSERT_FALSE(predictor.predict(1600).expired);
  const auto expired = predictor.predict(1601);
  TEST_ASSERT_TRUE(expired.expired && expired.shouldStop(50.f));
  TEST_ASSERT_FALSE(expired.shouldStop(-1.f));
  TEST_ASSERT_FLOAT_WITHIN(0.0001, 33.f, expired.finalWeightG); // no unbounded extrapolation
}

void test_shot_stop_handles_bootstrap_tare_and_hard_weight_limit(void) {
  ShotStopPredictor predictor;
  TEST_ASSERT_FALSE(predictor.predict(1000).active);
  predictor.reset(1000);
  TEST_ASSERT_FALSE(predictor.predict(1500).expired);
  TEST_ASSERT_TRUE(predictor.predict(1501).shouldStop(36.f));
  TEST_ASSERT_TRUE(predictor.add(shotSample(36.1f, 1600, 0), 1600));
  TEST_ASSERT_FALSE(predictor.predict(1600).flowReady);
  TEST_ASSERT_TRUE(predictor.predict(1600).shouldStop(36.f));
  predictor.reset(1700); // successful tare resets both mass origin and flow
  TEST_ASSERT_TRUE(predictor.add(shotSample(0.f, 1700, 0), 1700));
  TEST_ASSERT_FALSE(predictor.predict(1700).shouldStop(36.f));
  TEST_ASSERT_EQUAL_FLOAT(0.f, predictor.predict(1700).flowGPerS);
}

void test_shot_stop_flow_tracks_changes_and_rejects_isolated_mass_spike(void) {
  ShotStopPredictor predictor;
  predictor.reset(0);
  // A raw mass spike spread over two displayed readings produces +,0,- rate
  // disturbances. The median flow must stay at the real 6 g/s throughout.
  for (uint32_t n = 1; n <= 12; ++n) {
    float weight = n * 0.6f;
    if (n == 6 || n == 7) weight += 1.3f;
    TEST_ASSERT_TRUE(predictor.add(shotSample(weight, n * 100), n * 100));
    if (n >= 4) TEST_ASSERT_FLOAT_WITHIN(0.001, 6.f, predictor.predict(n * 100).flowGPerS);
  }
  // A real reduction to 2 g/s is followed after two new flow intervals.
  predictor.add(shotSample(7.4f, 1300), 1300);
  predictor.add(shotSample(7.6f, 1400), 1400);
  TEST_ASSERT_FLOAT_WITHIN(0.001, 2.f, predictor.predict(1400).flowGPerS);
  predictor.add(shotSample(7.6f, 1500), 1500);
  predictor.add(shotSample(7.6f, 1600), 1600);
  TEST_ASSERT_EQUAL_FLOAT(0.f, predictor.predict(1600).flowGPerS);
}

void test_shot_stop_uses_effective_times_across_irregular_sampling(void) {
  ShotStopPredictor predictor;
  predictor.reset(0);
  const uint32_t times[] = {100, 210, 390, 490};
  const uint32_t delays[] = {0, 55, 90, 50};
  for (unsigned i = 0; i < 4; ++i)
    predictor.add(shotSample(4.f * (times[i] - delays[i]) / 1000.f, times[i], delays[i]), times[i]);
  const auto prediction = predictor.predict(600, 500);
  TEST_ASSERT_FLOAT_WITHIN(0.001, 4.f, prediction.flowGPerS);
  TEST_ASSERT_FLOAT_WITHIN(0.001, 2.4f, prediction.currentWeightG);
  TEST_ASSERT_FLOAT_WITHIN(0.001, 4.4f, prediction.finalWeightG);
}

void test_shot_stop_rejects_invalid_inputs_and_handles_timer_wrap(void) {
  ShotStopPredictor predictor;
  seedShotFlow(predictor, 6.f, UINT32_MAX - 149);
  TEST_ASSERT_FLOAT_WITHIN(0.001, 6.f, predictor.predict(200).flowGPerS);
  TEST_ASSERT_FALSE(predictor.add(shotSample(NAN, 200), 200));
  TEST_ASSERT_FALSE(predictor.add(shotSample(INFINITY, 200), 200));
  TEST_ASSERT_FALSE(predictor.add(shotSample(33.f, 150), 200)); // duplicate
  TEST_ASSERT_FALSE(predictor.add(shotSample(33.f, 100), 200)); // out of order
  TEST_ASSERT_FALSE(predictor.add(shotSample(33.f, 201), 200)); // future
  TEST_ASSERT_FALSE(predictor.add(shotSample(33.f, 200, 501), 200));
  TEST_ASSERT_TRUE(predictor.add(shotSample(0.f, 800), 800)); // real gap resets flow history
  TEST_ASSERT_FALSE(predictor.predict(800).flowReady);
}

void test_shot_stop_replaces_legacy_lookahead_for_local_physical_scales(void) {
  SensorState state = {};
  GlobalStopConditions stop;
  stop.weight = 36.f;
  state.shotWeight = state.weight = 35.f;
  state.smoothedWeightFlow = 10.f;
  state.smoothedPumpFlow = 100.f;
  state.shotStop.active = true;
  state.shotStop.measuredWeightG = 35.f;
  state.shotStop.finalWeightG = 35.9f;
  TEST_ASSERT_FALSE(stop.isReached(state, 5000)); // no second 0.5 s allowance
  state.shotStop.finalWeightG = 36.f;
  TEST_ASSERT_TRUE(stop.isReached(state, 5000));
  TEST_ASSERT_FALSE(stop.isReached(state, 999));
  state.shotStop.finalWeightG = 0.f;
  state.shotStop.expired = true;
  TEST_ASSERT_TRUE(stop.isReached(state, 5000)); // no switch to pumped-volume guessing
  stop.weight = -1.f;
  TEST_ASSERT_FALSE(stop.isReached(state, 5000));
  stop.time = 4000;
  TEST_ASSERT_TRUE(stop.isReached(state, 5000));
}

void test_shot_stop_keeps_nonlocal_modes_and_hard_limits_working(void) {
  SensorState state = {};
  GlobalStopConditions stop;
  stop.weight = 36.f;
  state.shotWeight = state.weight = 35.f;
  state.smoothedWeightFlow = 4.f;
  TEST_ASSERT_TRUE(stop.isReached(state, 5000)); // existing nonlocal path
  state.shotWeight = 37.f;
  state.smoothedWeightFlow = 0.f;
  TEST_ASSERT_TRUE(stop.isReached(state, 5000));
}

void runAllShotStopTests(void) {
  RUN_TEST(test_shot_stop_projects_mass_age_filter_delay_and_inflight_tail);
  RUN_TEST(test_shot_stop_bridges_rejected_samples_without_zeroing_flow);
  RUN_TEST(test_shot_stop_handles_bootstrap_tare_and_hard_weight_limit);
  RUN_TEST(test_shot_stop_flow_tracks_changes_and_rejects_isolated_mass_spike);
  RUN_TEST(test_shot_stop_uses_effective_times_across_irregular_sampling);
  RUN_TEST(test_shot_stop_rejects_invalid_inputs_and_handles_timer_wrap);
  RUN_TEST(test_shot_stop_replaces_legacy_lookahead_for_local_physical_scales);
  RUN_TEST(test_shot_stop_keeps_nonlocal_modes_and_hard_limits_working);
}

#ifdef STANDALONE_SHOT_STOP_TEST
void setUp() {}
void tearDown() {}
int main() {
  UNITY_BEGIN();
  runAllShotStopTests();
  return UNITY_END();
}
#endif
