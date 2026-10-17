#include "profiling_phases.h"
#include "mcu_comms.h"
#include "utils/test-utils.h"

void test_profile_serializer_works_correctly(void) {
  Phase phaseArray[] = {

  };

  Profile profile;
  profile.phases.push_back({ PHASE_TYPE::PHASE_TYPE_PRESSURE, Transition(0.f, 10.f, TransitionCurve::EASE_IN_OUT, 1000), -1,  PhaseStopConditions{.time = 1000} });
  profile.phases.push_back({ PHASE_TYPE::PHASE_TYPE_FLOW, Transition(10.f, 5.f, TransitionCurve::LINEAR), 2.f,  PhaseStopConditions{.weight = 10.f} });
  profile.phases.push_back({ PHASE_TYPE::PHASE_TYPE_FLOW, Transition(10.f, 5.f, TransitionCurve::LINEAR), 3.f,  PhaseStopConditions{.pressureAbove = 2.f} });
  ProfileSerializer serializer;

  std::vector<uint8_t> serializedProfile = serializer.serializeProfile(profile);

  Profile deserializedProfile;
  TEST_ASSERT_EQUAL(serializer.neededBufferSize(profile), serializedProfile.size());
  TEST_ASSERT_TRUE(serializer.deserializeProfile(serializedProfile, deserializedProfile));

  TEST_ASSERT_EQUAL_PROFILE(profile, deserializedProfile);
}

void test_empty_profile_roundtrips() {
  Profile profile;
  profile.globalStopConditions.weight = 42.f;
  ProfileSerializer serializer;
  auto bytes = serializer.serializeProfile(profile);
  Profile decoded;
  decoded.addPhase(Phase{});
  TEST_ASSERT_TRUE(serializer.deserializeProfile(bytes, decoded));
  TEST_ASSERT_EQUAL_PROFILE(profile, decoded);
}

void test_truncated_profile_does_not_replace_recipe() {
  Profile source;
  source.addPhase(Phase{});
  ProfileSerializer serializer;
  auto bytes = serializer.serializeProfile(source);
  Profile target;
  target.globalStopConditions.weight = 42.f;
  for (size_t size = 0; size < bytes.size(); size++) {
    std::vector<uint8_t> truncated(bytes.begin(), bytes.begin() + size);
    TEST_ASSERT_FALSE(serializer.deserializeProfile(truncated, target));
    TEST_ASSERT_EQUAL(0, target.phaseCount());
    TEST_ASSERT_EQUAL_FLOAT(42.f, target.globalStopConditions.weight);
  }
}

void test_profile_rejects_invalid_count_and_trailing_bytes() {
  Profile profile;
  ProfileSerializer serializer;
  auto bytes = serializer.serializeProfile(profile);
  size_t invalidCount = static_cast<size_t>(-1);
  memcpy(bytes.data(), &invalidCount, sizeof(invalidCount));
  TEST_ASSERT_FALSE(serializer.deserializeProfile(bytes, profile));
  bytes = serializer.serializeProfile(profile);
  bytes.push_back(0);
  TEST_ASSERT_FALSE(serializer.deserializeProfile(bytes, profile));
}

void runAllProfileSerializerTests(void) {
  RUN_TEST(test_profile_serializer_works_correctly);
  RUN_TEST(test_empty_profile_roundtrips);
  RUN_TEST(test_truncated_profile_does_not_replace_recipe);
  RUN_TEST(test_profile_rejects_invalid_count_and_trailing_bytes);
}
