"""Run boiler-fill and tray-guard regressions on the host, without hardware."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
UNITY = ROOT / '.pio/libdeps/test/Unity/src'
if not (UNITY / 'unity.c').exists():
    raise SystemExit('Unity is missing; install native test dependencies with pio pkg install -e test')

with tempfile.TemporaryDirectory(prefix='blackpill-fill-tests-') as tmp:
    for name, define, sources in (
        ('boiler_fill', 'STANDALONE_BOILER_FILL_TEST', ['boiler_fill', 'drip_tray_guard']),
        ('drip_tray_guard', 'STANDALONE_DRIP_TRAY_TEST', ['drip_tray_guard']),
        ('lcd_boiler_fill', 'STANDALONE_LCD_BOILER_FILL_TEST', []),
        ('water_level', 'STANDALONE_WATER_LEVEL_TEST', []),
        ('scales_filter', 'STANDALONE_SCALES_FILTER_TEST', ['scales_filter', 'drip_tray_guard']),
        ('shot_stop', 'STANDALONE_SHOT_STOP_TEST', ['shot_stop', 'profiling_phases', 'utils']),
    ):
        target = Path(tmp) / name
        subprocess.run([
            'clang++', '-std=c++17', '-fsanitize=address,undefined', '-g',
            '-I' + str(ROOT / 'lib/Common'), '-I' + str(UNITY), '-D' + define,
            '-I' + str(ROOT / 'tools/blackpill-scales/tests/shim'),
            str(ROOT / f'test/tests/test_{name}.cpp'),
            *(str(ROOT / f'lib/Common/{source}.cpp') for source in sources),
            '-x', 'c++', str(UNITY / 'unity.c'), '-o', str(target),
        ], check=True)
        subprocess.run([str(target)], check=True)
