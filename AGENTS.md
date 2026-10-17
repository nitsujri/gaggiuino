# Repository instructions

## Target hardware

- Work against the frozen `release/stm32-blackpill` generation.
- The installed controller is the CoffeeHat/GCP PCB v3.0 Low Voltage board with an STM32F411CE BlackPill, not a LEGO/hand-wired build.
- Treat the official PCBv3 wiring diagram as the hardware source of truth: <https://gaggiuino.github.io/schematics/stm32-lv/GCP_wiring_PCBv3_LV.png>.
- Use the `all-pcb-stlink` PlatformIO environment for the main firmware. It selects the PCB layout (`SINGLE_BOARD`) and MAX31855 thermocouple support.
- Physical scales are installed using the Gaggiuino compact dual-HX711 scales board with an FPC load-cell connector. The dual-HX711 board connects to CoffeeHat PCBv3 J8, whose pins are `D2`, `5V`, `GND`, `CLK`, and `D1`.
- The installed dual-HX711/FPC board uses the normal shared-clock, two-data-channel configuration. Leave `SINGLE_HX711_BOARD` undefined; despite its name, that flag is only for one ordinary HX711 module. Do not change this unless the scale electronics are physically replaced.
- Do not use either `*-forced-predictive-stlink` environment and do not define `FORCE_PREDICTIVE_SCALES`.
- Automatic idle pressure release uses a fail-closed gross-weight drip-tray guard. Its machine-specific `DRIP_TRAY_ZERO_RAW_1`, `DRIP_TRAY_ZERO_RAW_2`, and `DRIP_TRAY_MIN_WEIGHT_G` values live in `extra_defines.ini`.
- The programmer is an ST-Link. Use SWD through the BlackPill's SWD connection; the PCBv3 J27 connector shown in the schematic is the programming/UART header (`BOOT`, `RESET`, `GND`, `TX`, `RX`, `3V3`), not a substitute for SWDIO/SWCLK.

## Local configuration

- Hardware-specific local flags and the default PlatformIO environment live in the gitignored `extra_defines.ini`.
- Keep `default_envs = all-pcb-stlink` unless the installed controller hardware changes.
- Do not select `lego-stlink`; it enables the LEGO relay behavior and uses a different thermocouple library.

## Build and verification

- PlatformIO is available as `pio` on PATH.
- Build the installed main configuration with `pio run` or explicitly with `pio run -e all-pcb-stlink`.
- After firmware-affecting changes, at minimum run `pio run -e all-pcb-stlink`.
- For scale-calibration changes, also run `pio run -e scales-calibration-stlink`.
- The normal main firmware binary is `.pio/build/all-pcb-stlink/firmware.bin`.
- Do not upload to connected hardware unless the user explicitly asks. When asked, use `pio run -e all-pcb-stlink -t upload` with the machine disconnected from mains power.

## Scale calibration

- The calibration firmware is a temporary image: `pio run -e scales-calibration-stlink -t upload`.
- Use the calibration TFT matching the installed display (`basic`, `discovery`, or `tjc`). The calibration sketch waits for the matching display UI to report `initCheck == 100`.
- Record both load-cell calibration factors. The calibration sketch does not persist them automatically.
- With the tray removed, the calibration firmware prints `DRIP_TRAY_ZERO_RAW_1` and `DRIP_TRAY_ZERO_RAW_2` for `extra_defines.ini`. Place the empty tray afterward to select a conservative minimum gross-weight threshold.
- Restore the normal display TFT, reflash `all-pcb-stlink`, and enter both factors in the firmware's scale settings.

## Safety and compatibility

- Treat PCBv3 signal names, connector pin order, and voltage labels exactly as shown in the linked schematic; do not infer wiring from newer Gaggiuino generations.
- Keep mains power disconnected while connecting or flashing the ST-Link. Do not use ST-Link power blindly; confirm the board's power arrangement and maintain a common ground.
- Do not mix current-generation release binaries, migration instructions, or pin mappings into this frozen STM32 BlackPill setup.
