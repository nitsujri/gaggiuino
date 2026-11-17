#include <cmath>
#include <unity.h>
#include "scales_filter.h"

namespace {
void expectScaleResult(ScaleSampleResult expected, ScaleSampleResult actual) {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(expected), static_cast<int>(actual));
}

void seedScaleFilter(ScalesFilter &filter, const long raw[2], const float factors[2], uint32_t start = 0) {
  for (uint32_t i = 0; i < 10; ++i)
    expectScaleResult(ScaleSampleResult::Accepted, filter.update(raw, factors, start + i * 100));
}
}

void test_scales_filter_rejects_captured_faults_without_poisoning_history(void) {
  ScalesFilter filter;
  const float factors[2] = {1966.f, -1986.f};
  const long normal[2] = {140542, -69532};
  seedScaleFilter(filter, normal, factors);
  // Captured channel-1 bit errors and the simultaneous all-ones frames.
  const long bit16[2] = {206078, -69532};
  const long bit13[2] = {132217, -70076};
  const long bothHigh[2] = {-1, -1};
  expectScaleResult(ScaleSampleResult::Outlier, filter.update(bit16, factors, 1000));
  expectScaleResult(ScaleSampleResult::Outlier, filter.update(bit16, factors, 1100));
  expectScaleResult(ScaleSampleResult::Accepted, filter.update(normal, factors, 1200));
  expectScaleResult(ScaleSampleResult::Outlier, filter.update(bit13, factors, 1300));
  expectScaleResult(ScaleSampleResult::Accepted, filter.update(normal, factors, 1400));
  expectScaleResult(ScaleSampleResult::Invalid, filter.update(bothHigh, factors, 1500));
  expectScaleResult(ScaleSampleResult::Invalid, filter.update(bothHigh, factors, 1600));
  expectScaleResult(ScaleSampleResult::Accepted, filter.update(normal, factors, 1700));
}

void test_scales_filter_confirms_load_placement_removal_and_redistribution(void) {
  ScalesFilter filter;
  const float factors[2] = {1000.f, -1000.f};
  const long normal[2] = {250000, 50000};
  const long cup[2] = {350000, 50000};
  const long redistributed[2] = {370000, 70000};
  seedScaleFilter(filter, normal, factors);
  expectScaleResult(ScaleSampleResult::Outlier, filter.update(cup, factors, 1000));
  expectScaleResult(ScaleSampleResult::Outlier, filter.update(cup, factors, 1100));
  expectScaleResult(ScaleSampleResult::LoadChanged, filter.update(cup, factors, 1200));
  expectScaleResult(ScaleSampleResult::Accepted, filter.update(cup, factors, 1300));
  expectScaleResult(ScaleSampleResult::Outlier, filter.update(redistributed, factors, 1400));
  expectScaleResult(ScaleSampleResult::Outlier, filter.update(redistributed, factors, 1500));
  expectScaleResult(ScaleSampleResult::LoadChanged, filter.update(redistributed, factors, 1600));
  expectScaleResult(ScaleSampleResult::Outlier, filter.update(normal, factors, 1700));
  expectScaleResult(ScaleSampleResult::Outlier, filter.update(normal, factors, 1800));
  expectScaleResult(ScaleSampleResult::LoadChanged, filter.update(normal, factors, 1900));
}

void test_scales_filter_preserves_flow_ramps_from_a_stationary_load(void) {
  const float factors[2] = {1000.f, -1000.f};
  const long normal[2] = {250000, 50000};
  const long increments[] = {600L, 1000L, 2000L};
  for (long increment : increments) {
    ScalesFilter filter;
    seedScaleFilter(filter, normal, factors);
    for (uint32_t n = 1; n <= 100; ++n) {
      const long flowing[2] = {normal[0] + increment * n, normal[1]};
      expectScaleResult(ScaleSampleResult::Accepted, filter.update(flowing, factors, 900 + n * 100));
    }
  }
}

void test_scales_filter_does_not_accept_repeated_inconsistent_spikes(void) {
  ScalesFilter filter;
  const float factors[2] = {1000.f, -1000.f};
  const long normal[2] = {250000, 50000};
  seedScaleFilter(filter, normal, factors);
  for (uint32_t n = 0; n < 100; ++n) {
    const long jump[2] = {n % 2 ? 350000 : 150000, 50000};
    expectScaleResult(ScaleSampleResult::Outlier, filter.update(jump, factors, 1000 + n * 100));
  }
  expectScaleResult(ScaleSampleResult::Accepted, filter.update(normal, factors, 11000));
}

void test_scales_filter_rejects_invalid_raw_and_calibration(void) {
  ScalesFilter filter;
  const float factors[2] = {1000.f, -1000.f};
  const long invalid[][2] = {{0, 50000}, {250000, 0}, {-8388608L, 50000},
    {250000, 8388607L}, {-1, -1}};
  uint32_t time = 0;
  for (const auto &pair : invalid) {
    expectScaleResult(ScaleSampleResult::Invalid, filter.update(pair, factors, time));
    time += 100;
  }
  const long normal[2] = {250000, 50000};
  const float invalidFactors[][2] = {{0, -1000}, {1000, 0}, {NAN, -1000},
    {1000, INFINITY}, {1.e-38f, -1000}};
  for (const auto &pair : invalidFactors) {
    expectScaleResult(ScaleSampleResult::Invalid, filter.update(normal, pair, time));
    time += 100;
  }
}

void test_scales_filter_resets_after_outage_and_handles_timer_wrap(void) {
  ScalesFilter filter;
  const float factors[2] = {1000.f, -1000.f};
  const long normal[2] = {250000, 50000}, changed[2] = {350000, 50000};
  const uint32_t start = UINT32_MAX - 1099;
  seedScaleFilter(filter, normal, factors, start);
  expectScaleResult(ScaleSampleResult::Outlier, filter.update(changed, factors, start + 1000));
  expectScaleResult(ScaleSampleResult::Outlier, filter.update(changed, factors, start + 1100));
  expectScaleResult(ScaleSampleResult::LoadChanged, filter.update(changed, factors, start + 1200));
  expectScaleResult(ScaleSampleResult::Accepted, filter.update(normal, factors, start + 1501));
  filter.reset();
  expectScaleResult(ScaleSampleResult::Accepted, filter.update(changed, factors, start + 1601));
}

void runAllScalesFilterTests(void) {
  RUN_TEST(test_scales_filter_rejects_captured_faults_without_poisoning_history);
  RUN_TEST(test_scales_filter_confirms_load_placement_removal_and_redistribution);
  RUN_TEST(test_scales_filter_preserves_flow_ramps_from_a_stationary_load);
  RUN_TEST(test_scales_filter_does_not_accept_repeated_inconsistent_spikes);
  RUN_TEST(test_scales_filter_rejects_invalid_raw_and_calibration);
  RUN_TEST(test_scales_filter_resets_after_outage_and_handles_timer_wrap);
}

#ifdef STANDALONE_SCALES_FILTER_TEST
void setUp() {}
void tearDown() {}
int main() {
  UNITY_BEGIN();
  runAllScalesFilterTests();
  return UNITY_END();
}
#endif
