#include <unity.h>
#include "water_level.h"

template <uint16_t FullDistanceMm>
void checkWaterLevelRange() {
  bool seen[21] = {};
  uint16_t previous = 100;
  for (uint32_t distance = 0; distance <= UINT16_MAX; ++distance) {
    const uint16_t percent = waterLevelPercent<FullDistanceMm>(distance);
    TEST_ASSERT_TRUE(percent >= 5 && percent <= 100 && percent % 5 == 0);
    TEST_ASSERT_TRUE(percent <= previous);
    TEST_ASSERT_EQUAL(distance <= FullDistanceMm, percent == 100);
    TEST_ASSERT_EQUAL(distance > 125, percent < 10);
    seen[percent / 5] = true;
    previous = percent;
  }
  for (unsigned step = 1; step <= 20; ++step) TEST_ASSERT_TRUE(seen[step]);
}

void test_water_level_full_tank_noise_stays_at_100(void) {
  TEST_ASSERT_EQUAL_UINT16(100, waterLevelPercent<28>(26));
  TEST_ASSERT_EQUAL_UINT16(100, waterLevelPercent<28>(27));
  TEST_ASSERT_EQUAL_UINT16(100, waterLevelPercent<28>(28));
  TEST_ASSERT_EQUAL_UINT16(95, waterLevelPercent<28>(29));
  TEST_ASSERT_EQUAL_UINT16(95, waterLevelPercent<28>(33));
  TEST_ASSERT_EQUAL_UINT16(90, waterLevelPercent<28>(34));
}

void test_water_level_preserves_low_water_cutoff(void) {
  TEST_ASSERT_EQUAL_UINT16(10, waterLevelPercent<28>(125));
  TEST_ASSERT_EQUAL_UINT16(5, waterLevelPercent<28>(126));
  TEST_ASSERT_EQUAL_UINT16(5, waterLevelPercent<28>(UINT16_MAX));
}

void test_water_level_all_distances_use_monotonic_five_percent_steps(void) {
  checkWaterLevelRange<15>();
  checkWaterLevelRange<28>();
}

void test_water_level_average_uses_only_last_five_seconds(void) {
  WaterLevelAverage average;
  TEST_ASSERT_EQUAL_UINT16(0, average.reading(0));
  average.add(20, 100);
  average.add(40, 1100);
  average.add(60, 2100);
  TEST_ASSERT_EQUAL_UINT16(40, average.reading(5099));
  TEST_ASSERT_EQUAL_UINT16(50, average.reading(5100));
  TEST_ASSERT_EQUAL_UINT16(60, average.reading(6100));
  TEST_ASSERT_EQUAL_UINT16(0, average.reading(7100));
  average.add(28, 10000);
  TEST_ASSERT_EQUAL_UINT16(28, average.reading(10000));
}

void test_water_level_average_handles_sustained_sampling_and_changes(void) {
  WaterLevelAverage average;
  // Exercise multiple trips around the ring and use all available startup data.
  for (uint32_t t = 0; t < 20000; t += 50) {
    average.add(28, t);
    TEST_ASSERT_EQUAL_UINT16(28, average.reading(t));
  }
  // Half the five-second history is now the new distance.
  for (uint32_t t = 20000; t < 22500; t += 50) average.add(48, t);
  TEST_ASSERT_EQUAL_UINT16(38, average.reading(22450));
  for (uint32_t t = 22500; t < 25000; t += 50) average.add(48, t);
  TEST_ASSERT_EQUAL_UINT16(48, average.reading(24950));
}

void test_water_level_average_limits_poll_rate_and_rounds_to_nearest_mm(void) {
  WaterLevelAverage average;
  average.add(27, 0);
  for (uint32_t t = 1; t < 50; ++t) average.add(1000, t);
  TEST_ASSERT_EQUAL_UINT16(27, average.reading(49));
  average.add(28, 50);
  TEST_ASSERT_EQUAL_UINT16(28, average.reading(50));
}

void test_water_level_average_handles_millis_wrap_and_large_distances(void) {
  WaterLevelAverage average;
  const uint32_t start = UINT32_MAX - 2499;
  average.add(20, start);
  average.add(40, start + 3000);
  TEST_ASSERT_EQUAL_UINT16(30, average.reading(start + 4999));
  TEST_ASSERT_EQUAL_UINT16(40, average.reading(start + 5000));
  TEST_ASSERT_EQUAL_UINT16(0, average.reading(start + 8000));
  for (uint32_t t = 10000; t < 20000; t += 50) average.add(UINT16_MAX, t);
  TEST_ASSERT_EQUAL_UINT16(UINT16_MAX, average.reading(19950));
}

void runAllWaterLevelTests(void) {
  RUN_TEST(test_water_level_full_tank_noise_stays_at_100);
  RUN_TEST(test_water_level_preserves_low_water_cutoff);
  RUN_TEST(test_water_level_all_distances_use_monotonic_five_percent_steps);
  RUN_TEST(test_water_level_average_uses_only_last_five_seconds);
  RUN_TEST(test_water_level_average_handles_sustained_sampling_and_changes);
  RUN_TEST(test_water_level_average_limits_poll_rate_and_rounds_to_nearest_mm);
  RUN_TEST(test_water_level_average_handles_millis_wrap_and_large_distances);
}

#ifdef STANDALONE_WATER_LEVEL_TEST
void setUp() {}
void tearDown() {}
int main() {
  UNITY_BEGIN();
  runAllWaterLevelTests();
  return UNITY_END();
}
#endif
