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

void runAllWaterLevelTests(void) {
  RUN_TEST(test_water_level_full_tank_noise_stays_at_100);
  RUN_TEST(test_water_level_preserves_low_water_cutoff);
  RUN_TEST(test_water_level_all_distances_use_monotonic_five_percent_steps);
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
