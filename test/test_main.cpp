#include <unity.h>
#include "./tests/test_pressure_profiler.cpp"
#include "./tests/test_pump.cpp"
#include "./tests/test_profile_serializer.cpp"
#include "./tests/test_drip_tray_guard.cpp"
#include "./tests/test_indigo.cpp"
#include "./tests/test_cleaning.cpp"

int main(int argc, char **argv) {
    UNITY_BEGIN();
    runAllPressureProfilerTests();
    runAllPumpTests();
    runAllProfileSerializerTests();
    runAllDripTrayGuardTests();
    IndigoTest::run();
    CleaningTest::run();
    UNITY_END();
}
