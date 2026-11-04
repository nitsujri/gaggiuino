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
    ):
        target = Path(tmp) / name
        subprocess.run([
            'clang++', '-std=c++17', '-fsanitize=address,undefined', '-g',
            '-I' + str(ROOT / 'lib/Common'), '-I' + str(UNITY), '-D' + define,
            str(ROOT / f'test/tests/test_{name}.cpp'),
            *(str(ROOT / f'lib/Common/{source}.cpp') for source in sources),
            '-x', 'c++', str(UNITY / 'unity.c'), '-o', str(target),
        ], check=True)
        subprocess.run([str(target)], check=True)
