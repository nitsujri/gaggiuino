#include <unity.h>
#include "./tests/test_pressure_profiler.cpp"
#include "./tests/test_pump.cpp"
#include "./tests/test_profile_serializer.cpp"
#include "./tests/test_drip_tray_guard.cpp"
#include "./tests/test_boiler_fill.cpp"
#include "./tests/test_lcd_boiler_fill.cpp"
#include "./tests/test_lcd_startup.cpp"
#include "./tests/test_water_level.cpp"
#include "./tests/test_scales_filter.cpp"
#include "./tests/test_shot_stop.cpp"

int main(int argc, char **argv) {
    UNITY_BEGIN();
    runAllPressureProfilerTests();
    runAllPumpTests();
    runAllProfileSerializerTests();
    runAllDripTrayGuardTests();
    runAllBoilerFillTests();
    runAllLcdBoilerFillTests();
    runAllLcdStartupTests();
    runAllWaterLevelTests();
    runAllScalesFilterTests();
    runAllShotStopTests();
    UNITY_END();
}
