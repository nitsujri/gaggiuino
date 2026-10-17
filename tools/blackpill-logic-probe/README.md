# Spare BlackPill USB logic probe

Separate STM32F411CE firmware for observing the installed controller's scales
signals. This is not Gaggiuino firmware and must **only be flashed to the spare**.
The main firmware, scale configuration, and calibration remain unchanged.

## Wiring

First program the spare with all probe wires disconnected. Disconnect machine
mains and both boards' power before adding or moving probe wires. Let cleaning
solvents dry completely. Leave the scales connected normally to J8 so the
installed controller supplies their clock. Connect in parallel at accessible
adapter pads, not the tiny HX711 pins:

| Spare BlackPill | Scales adapter / installed controller |
| --- | --- |
| GND | GND |
| PA0 (A0) | CLK (installed MCU PB0) |
| PA1 (A1) | D1 (installed MCU PB8) |
| PA2 (A2) | D2 (installed MCU PB9) |

The PA0/PA1/PA2 probe pins are digital inputs without pull-ups or pull-downs.
Analog acquisition temporarily configures PA0 as ADC1_IN0 with no pulls.
Optional 1 kΩ series resistors in each signal lead limit accidental current.
Use short wires and verify signal voltages are between GND and 3.3 V before
connecting. Do not attach any probe signal to the 5 V rail or a load-cell wire.

Power each BlackPill with its own USB cable to the same laptop. Connect common
GND, but **do not join either board's 3V3 or 5V pins**. If ST-Link remains
attached to a USB-powered board, leave its power-output lead disconnected.
Keep machine mains disconnected throughout these bench measurements.

## Build and program the spare

From this directory:

```sh
pio run -e probe-stlink
```

For ST-Link, move the programmer to the **spare's** SWD connector: DIO→DIO,
SCK→SCK, GND→GND. With the spare powered by USB and ST-Link power output
disconnected, verify which MCU the programmer targets, then:

```sh
pio run -e probe-stlink -t upload
```

Alternatively, connect only the spare by USB, hold BOOT0, press and release
NRST, then release BOOT0. Confirm the spare is the DFU target before:

```sh
pio run -e probe-dfu -t upload
```

This environment uses the native `dfu-util` on PATH and restricts uploads to
spare USB serial `349C39803133` (`probe.dfu_serial` in `platformio.ini`). Change
that identifier only if deliberately replacing the spare. The installed
controller's serial is `3563377A3534` and is not a permitted DFU target here.

Release BOOT0 and press NRST after programming if necessary. Probe inputs
must remain disconnected until this firmware is running on the spare.

## USB dashboard

Requires Python 3 and `pyserial` (already available in PlatformIO's Python):

```sh
/Users/justin/.platformio/penv/bin/python probe.py --list
/Users/justin/.platformio/penv/bin/python probe.py --port /dev/cu.usbmodemSPARE --output capture.json
```

Open <http://127.0.0.1:8765>. The service verifies the firmware's identifying
reply before requesting captures. **Run live** requests successive windows
without a deliberate delay; **Pause** freezes the displayed waveform immediately.
The probe still stops sampling during USB transfer, so this is not gapless
acquisition. **Single** acquires one window. The dashboard keeps the last 32
windows in History, preserves zoom between updates, and supports an explicit
time scale plus alignment to a captured CLK rising or falling edge. **Freeze
on CLK edge** pauses when either CLK edge is observed inside a captured window;
it cannot catch events during the gaps between windows.

Choose 250 ms or 1 second to see conversions at 10 samples/second; zoom to
inspect individual pulses. Only CLK is displayed by default; enable D1 and D2
once their wires are connected. A flat trace with no target attached is
meaningless because disconnected inputs float.

For a single machine-readable capture:

```sh
/Users/justin/.platformio/penv/bin/python probe.py --port /dev/cu.usbmodemSPARE --once --ms 250 --output capture.json
```

## What it measures

### Analog voltage capture on PA0

Choose **Analog PA0 voltage**, **Fastest: 4 MS/s · 6 bit**, and Run live.
Only PA0 is sampled in analog mode; the existing PA0→CLK and common GND
connections are sufficient. **The analog input is limited to GND..VDDA
(approximately 0–3.3 V). It is not 5 V tolerant in analog mode.**

The spare uses a 72 MHz CPU/APB2 clock, 48 MHz USB clock, and 36 MHz ADC
clock, all within STM32F411 limits. A normal DMA transfer fills a 32,768-word
buffer with uniformly spaced voltage samples, independent of USB/SysTick
interrupts. USB transmission happens after sampling stops. Capture RAM is
shared with the digital event buffer so both modes fit the MCU's SRAM.

| Preset | Resolution | ADC cycles/sample | Sample rate | 32,768-sample window |
| --- | --- | --- | --- | --- |
| Fastest | 6 bits | 3 + 6 | 4 MS/s | 8.192 ms |
| Detail | 12 bits | 3 + 12 | 2.4 MS/s | 13.653 ms |
| Long acquisition | 12 bits | 480 + 12 | 73.171 kS/s | 447.829 ms |

The first two rates assume a low-impedance source. The ADC's sample capacitor
loads the signal; use Long acquisition for high-impedance sources and compare
with a multimeter before interpreting a weak/floating node. This is an
unbuffered ADC, not a calibrated oscilloscope front end. Six bits provides
only 64 levels (about 52 mV per step). The voltage reference is estimated from
the factory VREFINT calibration and flagged if an assumed 3.3 V fallback is
used. Overview rendering preserves per-pixel minima/maxima; zoom displays
individual samples rather than inventing a smooth curve.

The probe rejects DMA errors, pre-completion ADC overruns, and acquisition
timeouts. Each analog frame includes the clock-derived sample rate and
CPU-cycle acquisition duration, so the achieved timing can be checked on the
actual spare after flashing. Analog data is sent as length-delimited little
endian 16-bit samples; the host validates resolution, count, and framing.

```sh
/Users/justin/.platformio/penv/bin/python probe.py --port /dev/cu.usbmodem349C398031331 --once --mode analog --preset 0 --count 32768 --output analog-capture.json
```

### Digital logic capture

The firmware polls all three GPIOs together and timestamps observed transitions
with the CPU cycle counter. It stores up to 8,190 transitions in RAM, then sends
them over USB after acquisition. A full buffer ends the capture early and is
explicitly reported. Capture durations are 10–1,000 ms.

Digital mode cannot measure voltage, rise time, overshoot, or explain an
intermediate voltage. USB and system
interrupts remain enabled and can hide pulses. The dashboard reports the largest
polling gap, which must be compared with the roughly 2 µs clock pulses generated
by this frozen Gaggiuino HX711 driver. No observed edge is not proof of no edge.

Before interpreting the machine, check the probe disconnected from it:
connecting a probe input to the **spare's own GND** should read 0, and to its
**own 3V3** should read 1. Never use its 5 V pin. These static checks do not
validate fast-pulse capture; that still needs a known pulse source.

Host protocol checks: `python3 -m unittest discover -s tests -v`.
Dashboard control-state checks: `node tests/test_dashboard.cjs`.

References: [designer's FPC board](https://github.com/banoz/dualScaleBoard_HX711/tree/feature/fpc),
[STM32F411 datasheet](https://www.st.com/resource/en/datasheet/stm32f411ve.pdf),
[Arm Cortex-M4 cycle counter](https://documentation-service.arm.com/static/5f19da2a20b7cf4bc524d99a).
