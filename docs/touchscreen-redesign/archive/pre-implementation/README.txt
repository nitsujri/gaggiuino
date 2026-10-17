Indigo redesign: audit and implementation package

Deployment status: NOT READY. No native Indigo HMI/TFT or matching controller
extensions/migration exist yet. Existing machine has not been flashed.

Read AUDIT.txt first: findings, fixes, gaps, and verification limits.
SPECIFICATION.txt: layout, behavioral parity, protocol, EEPROM and guard contract.
feature-matrix.csv:47 detailed behavior comparisons and implementation status.
field-matrix.csv:75 original stored fields, including55 per profile.
ACCEPTANCE.csv:61 release checks; physical/native integration still NOT RUN.
inventory.json: all19 original pages,413 components,234 event scripts,
serial addresses/triggers, source locations, build flags and backup hashes.
release-status.json: explicitly nondeployable artifact/status manifest.
LOADING.txt: eventual two-part installation and rollback; explains what not
to load today and why. ST-Link SWD normally needs neither BOOT nor NRST.

Selected browser preview:
    http://127.0.0.1:8767/touchscreen-indigo/
Files:
    tmp/touchscreen-indigo/
If server is stopped, start from repository root:
    python3 -m http.server 8767 --bind 127.0.0.1 --directory tmp

Read-only inventory regeneration:
    python3 tools/touchscreen-audit/audit.py
Original HMI component/event fixture is in tools/touchscreen-audit/.

Controller fixes are in the working tree, not uploaded or committed. Main
all-pcb-stlink build passes.23 native cases pass under address/undefined
sanitizers;557 browser checks pass. These do not prove native display parity.
See AUDIT.txt for commands, original ArduinoFake/Clang compatibility issue,
remaining sensor/guard/storage defects and hardware acceptance requirements.
