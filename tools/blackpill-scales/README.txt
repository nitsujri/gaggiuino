BlackPill scale recovery and drip-tray guard

Water gauge calibration: at the preferred full water level,40 live samples over
34s measured26-28mm (median27mm). WATER_TANK_FULL_DISTANCE_MM=28 includes the
observed noise. The gauge uses evenly spaced5% steps from100% at28mm to10% at
125mm; distances beyond125mm report5% and retain the existing low-water cutoff.
No display TFT update is needed. These are calibrated level marks, not a
measurement of water volume.
The gauge averages distance samples from the last5s before converting to5%
steps, using available samples immediately at startup. Samples expire by age,
and polling continues on all display pages. A current distance beyond125mm
still reports5% immediately, so averaging never delays the low-water interlock.

This branch retains the frozen stock display and schema12 settings. The scale
clock remains shared/open-drain; the installed board requires the internal
pull-up and 16us half-period. The same setup is used by main and calibration.

The HX711 source is vendored from the previously pinned dependency with a small
atomic nonblocking read extension. The guard uses only fresh local conversions
(maximum age300ms), rejects zero/saturated data and invalid calibration factors,
and measures gross load against a separately measured no-tray baseline. Display
tare and remote/predictive weights cannot make the tray present.

Scale filtering (prepared2026-11-16): each raw channel is checked against the
last5 accepted conversions using its median and median absolute deviation,
with a1g per-channel noise floor. A rate allowance of20g/s per channel lets
normal water-flow ramps start from a stationary load. Large jumps require3
consistent new conversions before rebasing for a real cup/load change. Zero,
saturated, simultaneous all-ones(-1,-1) frames and invalid scale factors are
rejected outright. This cannot distinguish every persistent hardware fault
from a genuine sustained load change; it suppresses the captured transient
faults without claiming to repair the electronics.

Only accepted data enters a2-conversion display average (about50ms extra lag
at10Hz). Tare uses that displayed raw mean and cannot change absolute safety
weights. A rejected conversion preserves the old display timestamp, clears the
4-conversion safety window and immediately blocks tray/boiler-fill permission.
Continuous rejection makes the held display data stale after300ms; recovery
requires4 consecutive accepted, settled readings. The startup +/-0.5g baseline
tolerance and sustained3g water-arrival confirmation remain unchanged.

The64-byte SWD snapshot retains its old32-byte prefix and adds observed raw
channels, last observation time, rejection/load-change counters, filter status
and displayed raw means. Use the updated read-scales.py with the matching new
ELF only after this firmware is flashed. settledFresh matches the actual5g
four-conversion readiness limit; startup baseline settling is a separate check.

The production sampler regression replays290 coherent observations captured
from the installed machine, with their observed timestamp gaps, in
tests/data/jumpy-2026-11-16.csv. The captured gross range was139.83g; filtered
display range is1.56g with no false confirmed load changes. It also checks
real load placement/removal,6/10/20g/s ramps, tare, stale and rejected samples,
and blocked boiler confirmation under corruption, using ASan+UBSan. Live
monitoring of this filter has now completed: a10-minute mains-off run rejected89
of6335 conversions (1.4%), with no false load changes or resets. Slow1.11g drift
on channel2 and smaller residual excursions remained. Powered brewing acceptance
is still pending; no hardware write is part of the host regressions.

Physical-scale shot stopping: the local ADC path now estimates output flow as
the median of3 signed mass/time intervals (4 distinct accepted observations).
Repeated cached weights do not advance the estimator or collapse flow to zero.
The2-reading weight average retains its actual midpoint delay, including an
irregular accepted-sample interval after rejection. Each main-loop prediction is:
  current mass = last measured mass + flow * (sample age + smoothing delay)
  final mass = current mass + flow * SHOT_STOP_TAIL_MS /1000
The flow estimate is recomputed only on new accepted observations and bounded
to0..20g/s. The local stop checks final mass >= target and also retains the
direct measured-mass limit. It replaces the legacy0.5s look-ahead for local
hardware scales, so compensation is not applied twice. Remote/pump-estimated
scale modes retain their prior stop path; no forced-predictive flag is enabled.

SHOT_STOP_TAIL_MS=500 is an initial equivalent liquid-arrival allowance for
in-flight liquid, valve response and residual discharge, preserving the old
nominal0.5s allowance. It must be tuned from actual final cup weights; an idle
test cannot identify that machine parameter. The predictor bridges accepted
data gaps up to500ms. Longer gaps request a stop for a weight-controlled shot
(subject to the existing first-second minimum-shot guard), with no automatic
switch to estimated pumped weight. A failed tare remains pending until a fresh
accepted conversion permits it; a successful tare resets the mass/flow model.
Local weight-controlled shots establish that zero before starting the pump.
Tray presence and startup boiler filling continue using their existing settled
absolute-weight checks, without projected weight. No EEPROM or display protocol
layout changes are required. The source SensorState adds internal prediction
fields; serialized ShotSnapshot and SensorStateSnapshot layouts are unchanged.

Shot-stop regressions cover measured timestamps, filtering delay, tail allowance,
cached/rejected samples, timer wrap, changing/zero flow and invalid inputs. The
production-sampler tests simulate2/6/10g/s pours to36g, including3 rejected ADC
frames across the ideal cutoff; each reaches36g under the assumed0.5s tail model.
These are model tests, not measured final-yield accuracy. Live brew calibration
and acceptance of the new stop model remain pending.

The full native suite currently passes60 tests. With the current Apple Clang,
the old ArduinoFake dependency needs this test-only compatibility flag:
  PLATFORMIO_BUILD_FLAGS='-std=gnu++17 -Wno-missing-template-arg-list-after-template-kw' pio test -e test
This flag is not applied to either STM32 firmware build.

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

Startup boiler fill now confirms water arrival using the local four-conversion
mean, independent of display tare and remote/predictive scales. Before pumping,
it waits at least3s and requires a baseline within +/-0.5g (<=1g total spread)
for1s. This tolerance applies only to startup tray settling/boiler filling;
normal scale readings and the idle discharge guard are unchanged. It then
pumps at the existing35 setting until the tray gains at least3g continuously
for1s, with at least2s of pumping. Existing water in the tray is part of the
baseline and cannot complete filling. Brief spikes, dips below3g, cached
conversions and stale/invalid readings cannot complete the confirmation.
The pressure plateau cutoff and old8s overall timeout are removed. The new
limit is30s of pumping; reaching it stops the pump and closes the valve, shows
"Boiler fill timed out!", and leaves startup/heating disabled until reboot.
Manual steam-page flushing remains available after a timeout. Warm starts also
require water confirmation; temperature cannot establish boiler fullness after
steaming. Tray/switch/water/page interlocks still stop
the pump and reset the fill baseline; idle pressure release waits for startup
to finish so it cannot interrupt the longer fill.

The filling message now follows the pump command for the entire fill. The stock
popup's local one-second return timer is disabled only while filling, then
restored when filling completes, pauses or fails. A tray/sensor interruption
clears the message immediately; a retry shows it again when pumping resumes.
Pump transitions log their timestamp and reason (tray/machine interlock,
settling scales, water confirmation or timeout). The host display regression
models the stock timer, text clearing and a pause followed by a delayed retry.
No display TFT update is required.

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

Boiler-fill and guard/clock regressions run with ASan+UBSan using Unity from
the PlatformIO native test dependency cache:
  python3 tools/blackpill-scales/tests/run_boiler_fill.py
The regular test runner also includes these tests. The production sampler
startup checks below also simulate delayed water arrival and tare during fill.

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
