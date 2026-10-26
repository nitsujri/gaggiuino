BlackPill scale recovery and drip-tray guard

This branch retains the frozen stock display and schema12 settings. The scale
clock remains shared/open-drain; the installed board requires the internal
pull-up and 16us half-period. The same setup is used by main and calibration.

The HX711 source is vendored from the previously pinned dependency with a small
atomic nonblocking read extension. The guard uses only fresh local conversions
(maximum age300ms), rejects zero/saturated data and invalid calibration factors,
and measures gross load against a separately measured no-tray baseline. Display
tare and remote/predictive weights cannot make the tray present.

Enable DRIP_TRAY_GUARD_ENABLED=1 and set both DRIP_TRAY_ZERO_RAW_1 and
DRIP_TRAY_ZERO_RAW_2 in ignored extra_defines.ini. The calibrated setting is205g,75% of the separately weighed273.45g tray,
rounded down. See calibration-2026-10-17.json for raw measurements and factors. Unconfigured,
missing, stale or faulty readings block automatic boiler fill and idle pressure
release. The fill timer restarts after a blocked interval, so waiting for a tray
cannot silently skip boiler filling. The stock steam-page manual flush bypasses
the tray guard and remains available while waiting for startup boiler fill.
The tray interlock never shows a popup, including for missing, unsettled, stale,
faulty or unconfigured readings, so menus remain accessible. Missing trays and
normal startup settling are silent; other guard faults are recorded in logs.
Existing backflush/descale programs are unchanged.

Low-voltage checks require mains disconnected and the established SWD/power
arrangement. Read diagnostics without changing the display or flashing temporary
calibration firmware:
  python3 tools/blackpill-scales/read-scales.py --output /path/to/capture.json

Use the exact ELF matching the running firmware (default is the main build).
The script reads the scale snapshot and HAL clock without halting the core,
so the timer-driven HX711 clock continues uninterrupted. It never writes flash. Do not run multiple ST-Link tools concurrently.

Only with the tray and ALL containers physically removed may settledFresh=true
rawMean values be used as the two no-tray offsets. Take repeated captures and
check stability before saving those compile flags and rebuilding/uploading.
Then refit the empty tray, verify grossWeightG comfortably exceeds the threshold
and trayStatus=Present, remove it and verify Missing. Tare with the tray present
and verify gross weight/classification remain unchanged. Physical actuation
acceptance still requires a separate supervised machine test.

Build sequentially:
  pio run -e all-pcb-stlink
  pio run -e scales-calibration-stlink
Copy the main artifacts before changing environments if needed.

Ten isolated guard/clock tests can be run with ASan+UBSan using Unity from the
PlatformIO native test dependency cache. Define STANDALONE_DRIP_TRAY_TEST when
compiling test/tests/test_drip_tray_guard.cpp with lib/Common/drip_tray_guard.cpp
and Unity. The regular test runner also includes these tests.

Starter recipes are in starter-recipes.json and firmware factory defaults:
  Slot1 Turbo:15g ->45g,92C,constant6bar,6ml/s flow ceiling,no PI/soak.
  Slot2 SOUP:17g ->51g(1:3),86C,2s wetting2.5ml/s capped1bar,
             8s soak holding1bar(max2.5ml/s),6ml/s extraction capped1bar.
The chosen SOUP parameters adapt the cited gentle wetting/fast-flow method to
this frozen engine; they are not a canonical recipe. Slots3..5 and machine
calibrations are preserved when provisioning the current machine. Merely
changing factory defaults does not overwrite profiles already stored in EEPROM.

Calibration2026-10-17: factors1966 and-1986 saved in machine EEPROM. Centered
10/20/30/50g weights plus50g over left and right supports determined both gains;
the99.94g independent check predicted99.80g. Final no-tray offsets are-140084
and191846. The guard also requires four settled fresh conversions (spread<=5g),
so a transient high reading cannot enable an automatic discharge. Saved flags
are in extra_defines.pcb-v3-calibrated.example.ini; actual extra_defines.ini
remains gitignored. Calibration data and full flash backups are retained locally.

Startup handling (prepared2026-10-18): upstream waited up to1000ms for ADC
readiness and then tared four conversions. That low-level sequence remains.
The new tray guard additionally requires four fresh, settled cached samples.
During the first3s after scale initialization, that normal sample accumulation
is reported internally as Settling, with automatic actions still blocked and
no fault popup. Valid data is usable immediately; the3s window is not a fixed
delay. An ADC detection timeout remains a real error, and stale/unsettled
readings after the window still fail closed. Error logs now include sample
count, age and spread. This update was flashed and verified2026-10-18; cold-boot acceptance is pending.

Production sampler startup regression checks with mocked ADC hardware:
  python3 tools/blackpill-scales/tests/run_startup.py
This runs configured and unconfigured cases under ASan+UBSan, exercising the
actual src/peripherals/scales.cpp sampler as well as the pure guard helper.

Startup build label (prepared2026-10-19): LCD initialization resets retained UI
modes, opens the stock splash with its countdown paused, writes AUTO_VERSION,
and verifies the text by reading it back before restarting the countdown. Slow
display startup or a lost first write is retried while machine outputs remain
off. The old wait for a one-shot boot banner is removed. The existing TFT and
short commit identifier are retained; no display reflash is needed. Four host
regressions cover delayed boot, MCU-only reset, lost writes and wrong readback.
