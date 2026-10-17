#include <cmath>
#include <unity.h>

#include "drip_tray_guard.h"

void test_drip_tray_gross_weight_survives_tare_changes(void) {
  const long noTrayOffsets[2] = { 100000L, 200000L };
  const float scaleFactors[2] = { 4000.f, 3800.f };

  const long firstTaredRaw[2] = { 4000L, 8000L };
  const long firstOffsets[2] = { 120000L, 230000L };
  const float firstWeight = calculateDripTrayGrossWeight(
    firstTaredRaw,
    firstOffsets,
    noTrayOffsets,
    scaleFactors
  );

  const long secondTaredRaw[2] = { -6000L, -2000L };
  const long secondOffsets[2] = { 130000L, 240000L };
  const float secondWeight = calculateDripTrayGrossWeight(
    secondTaredRaw,
    secondOffsets,
    noTrayOffsets,
    scaleFactors
  );

  TEST_ASSERT_FLOAT_WITHIN(0.01f, 16.f, firstWeight);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, firstWeight, secondWeight);
}

void test_drip_tray_gross_weight_supports_signed_calibration(void) {
  const long taredRaw[2] = { -4000L, 5000L };
  const long offsets[2] = { 0L, 0L };
  const long noTrayOffsets[2] = { 0L, 0L };
  const float scaleFactors[2] = { -4000.f, 5000.f };

  TEST_ASSERT_FLOAT_WITHIN(
    0.01f,
    2.f,
    calculateDripTrayGrossWeight(taredRaw, offsets, noTrayOffsets, scaleFactors)
  );
}

void test_drip_tray_invalid_scale_factor_is_not_a_weight(void) {
  const long values[2] = { 1L, 1L };
  const float scaleFactors[2] = { 4000.f, 0.f };

  TEST_ASSERT_TRUE(std::isnan(calculateDripTrayGrossWeight(values, values, values, scaleFactors)));
}

void test_drip_tray_status_fails_closed(void) {
  TEST_ASSERT_EQUAL_INT(
    static_cast<int>(DripTrayStatus::Unconfigured),
    static_cast<int>(classifyDripTray(false, true, true, 500.f, 100.f))
  );
  TEST_ASSERT_EQUAL_INT(
    static_cast<int>(DripTrayStatus::Unavailable),
    static_cast<int>(classifyDripTray(true, false, true, 500.f, 100.f))
  );
  TEST_ASSERT_EQUAL_INT(
    static_cast<int>(DripTrayStatus::Unavailable),
    static_cast<int>(classifyDripTray(true, true, false, 500.f, 100.f))
  );
  TEST_ASSERT_EQUAL_INT(
    static_cast<int>(DripTrayStatus::Unconfigured),
    static_cast<int>(classifyDripTray(true, true, true, 500.f, 0.f))
  );
}

void test_drip_tray_status_uses_minimum_weight(void) {
  TEST_ASSERT_EQUAL_INT(
    static_cast<int>(DripTrayStatus::Missing),
    static_cast<int>(classifyDripTray(true, true, true, 99.9f, 100.f))
  );
  TEST_ASSERT_EQUAL_INT(
    static_cast<int>(DripTrayStatus::Present),
    static_cast<int>(classifyDripTray(true, true, true, 100.f, 100.f))
  );
}

void runAllDripTrayGuardTests(void) {
  RUN_TEST(test_drip_tray_gross_weight_survives_tare_changes);
  RUN_TEST(test_drip_tray_gross_weight_supports_signed_calibration);
  RUN_TEST(test_drip_tray_invalid_scale_factor_is_not_a_weight);
  RUN_TEST(test_drip_tray_status_fails_closed);
  RUN_TEST(test_drip_tray_status_uses_minimum_weight);
}
