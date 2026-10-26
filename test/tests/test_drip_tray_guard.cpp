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

void test_drip_tray_rejects_invalid_or_stale_samples(void) {
  TEST_ASSERT_FALSE(dripTraySampleFresh(false, 100, 100));
  TEST_ASSERT_TRUE(dripTraySampleFresh(true, 100, 400));
  TEST_ASSERT_FALSE(dripTraySampleFresh(true, 100, 401));
  TEST_ASSERT_TRUE(dripTraySampleFresh(true, UINT32_MAX - 99, 100));
  const long valid[2] = {-1000, 2000}, zero[2] = {0, 2000};
  const long lowRail[2] = {-8388608L, 2000}, highRail[2] = {1000, 8388607L};
  TEST_ASSERT_TRUE(dripTrayRawPairValid(valid));
  TEST_ASSERT_FALSE(dripTrayRawPairValid(zero));
  TEST_ASSERT_FALSE(dripTrayRawPairValid(lowRail));
  TEST_ASSERT_FALSE(dripTrayRawPairValid(highRail));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(DripTrayStatus::Unavailable),
    static_cast<int>(classifyDripTray(true, true, true, NAN, 150)));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(DripTrayStatus::Unconfigured),
    static_cast<int>(classifyDripTray(true, true, true, 200, INFINITY)));
}

void test_boiler_fill_timeout_restarts_after_waiting_for_tray(void) {
  BoilerFillClock clock;
  TEST_ASSERT_EQUAL_UINT32(0, clock.elapsed(10000));
  TEST_ASSERT_EQUAL_UINT32(4000, clock.elapsed(14000));
  clock.reset();
  TEST_ASSERT_EQUAL_UINT32(0, clock.elapsed(60000));
  TEST_ASSERT_EQUAL_UINT32(8000, clock.elapsed(68000));
  clock.reset();
  TEST_ASSERT_EQUAL_UINT32(0, clock.elapsed(UINT32_MAX - 99));
  TEST_ASSERT_EQUAL_UINT32(200, clock.elapsed(100));
}

void test_drip_tray_transient_cannot_enable_automatic_discharge(void) {
  TEST_ASSERT_FALSE(dripTrayReadingsSettled(3, 0.f));
  TEST_ASSERT_TRUE(dripTrayReadingsSettled(4, 5.f));
  TEST_ASSERT_FALSE(dripTrayReadingsSettled(4, 5.01f));
  TEST_ASSERT_FALSE(dripTrayReadingsSettled(4, NAN));
  TEST_ASSERT_FALSE(dripTrayReadingsSettled(4, INFINITY));
  TEST_ASSERT_FALSE(dripTrayReadingsSettled(4, -1.f));
}

void test_drip_tray_waits_for_boot_samples_without_calling_them_an_error(void) {
  TEST_ASSERT_TRUE(dripTrayStartupPending(true, false, 1000, 1000));
  TEST_ASSERT_TRUE(dripTrayStartupPending(true, false, 1000, 3999));
  TEST_ASSERT_FALSE(dripTrayStartupPending(true, false, 1000, 4000));
  // Actual ADC detection failures are errors, even during the settling window.
  TEST_ASSERT_FALSE(dripTrayStartupPending(false, false, 1000, 1100));
  // Good readings permit the normal tray check immediately; no fixed extra wait.
  TEST_ASSERT_FALSE(dripTrayStartupPending(true, true, 1000, 1100));
  TEST_ASSERT_NOT_EQUAL(static_cast<int>(DripTrayStatus::Present),
                        static_cast<int>(DripTrayStatus::Settling));
}

void test_drip_tray_startup_window_handles_clock_wrap(void) {
  TEST_ASSERT_TRUE(dripTrayStartupPending(true, false, UINT32_MAX - 999, 1999));
  TEST_ASSERT_FALSE(dripTrayStartupPending(true, false, UINT32_MAX - 999, 2000));
}

void runAllDripTrayGuardTests(void) {
  RUN_TEST(test_drip_tray_gross_weight_survives_tare_changes);
  RUN_TEST(test_drip_tray_gross_weight_supports_signed_calibration);
  RUN_TEST(test_drip_tray_invalid_scale_factor_is_not_a_weight);
  RUN_TEST(test_drip_tray_status_fails_closed);
  RUN_TEST(test_drip_tray_status_uses_minimum_weight);
  RUN_TEST(test_drip_tray_rejects_invalid_or_stale_samples);
  RUN_TEST(test_boiler_fill_timeout_restarts_after_waiting_for_tray);
  RUN_TEST(test_drip_tray_transient_cannot_enable_automatic_discharge);
  RUN_TEST(test_drip_tray_waits_for_boot_samples_without_calling_them_an_error);
  RUN_TEST(test_drip_tray_startup_window_handles_clock_wrap);
}

#ifdef STANDALONE_DRIP_TRAY_TEST
void setUp() {}
void tearDown() {}
int main() {
  UNITY_BEGIN();
  runAllDripTrayGuardTests();
  return UNITY_END();
}
#endif
