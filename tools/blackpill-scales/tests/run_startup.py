#!/usr/bin/env python3
"""Test the production scale startup path on the host; never accesses hardware."""
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='blackpill-startup-tests-') as tmp:
    command=['clang++','-std=c++17','-fsanitize=address,undefined','-g',
             '-I'+str(HERE/'shim'),'-I'+str(ROOT),'-I'+str(ROOT/'lib/Common'),'-I'+str(ROOT/'src'),
             str(ROOT/'src/peripherals/scales.cpp'),str(ROOT/'lib/Common/drip_tray_guard.cpp'),
             str(ROOT/'lib/Common/boiler_fill.cpp'),
             str(ROOT/'lib/Common/scales_filter.cpp'),
             str(ROOT/'lib/Common/shot_stop.cpp'),
             str(ROOT/'lib/Common/profiling_phases.cpp'),str(ROOT/'lib/Common/utils.cpp'),
             str(HERE/'test_scales_startup.cpp')]
    for configured in (True,False):
        target=Path(tmp)/('configured' if configured else 'unconfigured')
        flags=['-DDRIP_TRAY_ZERO_RAW_1=100000L','-DDRIP_TRAY_ZERO_RAW_2=200000L','-DDRIP_TRAY_MIN_WEIGHT_G=205.f'] if configured else []
        subprocess.run(command+flags+['-o',str(target)],check=True)
        subprocess.run([str(target),str(HERE/'data/jumpy-2026-11-16.csv')],check=True)
