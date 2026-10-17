CURRENT HARDWARE: PRE-INDIGO BACKUP RESTORED AND VERIFIED
User requested rollback after being unable to prepare a display SD card.
The exact512KB pre-Indigo controller/settings image was restored; full readback
matches SHA256 5eaf7905ee786803836b3324ac386e57c8fe46137ed8e2ff0fce4dab0f8ce400.
The display has never been changed. This restores the original sensor-only
TESTING configuration, including its earlier actuator/interlock bypass setup.
Evidence: tmp/indigo-rollback/restore-result.json. Indigo source/build artifacts
remain experimental work, not current installed firmware. A normal pio upload
from this working tree would build Indigo; use the documented verified backup
for another exact restoration. TJC compiler work has been stopped.

STATUS CORRECTION — TJC3224T028 CONFIRMED
The controller image has been programmed and verified, but startup is still
waiting for the touchscreen. The previous unconditional no-TFT/ready-to-load
claims below are withdrawn. A paired TJC3224T028 display build is now required
for this deployment; native UART rendering has not been verified on this panel.
The exact previous working controller/settings are backed up. No new TFT has
yet been compiled or installed. See release-status.json for the current status.

INDIGO — CONTROLLER IMPLEMENTATION AND FIRST-LOAD PACKAGE

Indigo is implemented for the frozen STM32 BlackPill / PCBv3 generation.
It draws natively on the existing stock touchscreen using UART commands.
There is NO display update: keep the currently installed TFT. Original
brewGraph, graphPreview and the manual slider remain on that display.

Software checks pass; physical touchscreen and machine acceptance are pending.
No controller or display has been flashed during this implementation.
The main image is PRODUCTION firmware: boiler filling and temperature fault
handling are enabled. The historical installed backup was a sensor-only bench
image. Loading Indigo restores real pump/heater behavior when mains is used.

LOADING.txt                concrete load, first boot and rollback procedure
SPECIFICATION.txt          implemented behavior, storage and transport contract
AUDIT.txt                  parity findings, changes, evidence and limitations
feature-matrix.csv         original behavior mapped to current implementation
field-matrix.csv           original fields and new schema13 fields
ACCEPTANCE.csv             software evidence versus physical checks not run
inventory.json             original HMI inventory and current source hashes
release-status.json        artifact hashes and verification status

Built files and logs: tmp/indigo-release/
Actual C++ renderer previews: tmp/indigo-native-preview/contact-sheet.png
Earlier HTML design reference: tmp/touchscreen-indigo/ (not installable)
Historical pre-implementation audit: archive/pre-implementation/

Reproduce read-only inventory: python3 tools/touchscreen-audit/audit.py
See LOADING.txt for exact build and sanitized test commands. Run PlatformIO
commands sequentially; changing environments can remove previous build outputs.
