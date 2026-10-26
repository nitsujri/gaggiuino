#!/usr/bin/env python3
"""Read the non-actuating scale diagnostic snapshot over SWD. Never writes flash."""
import argparse
import json
import math
from pathlib import Path
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
CLI = '/Applications/STMicroelectronics/STM32Cube/STM32CubeProgrammer/STM32CubeProgrammer.app/Contents/MacOs/bin/STM32_Programmer_CLI'
NM = Path.home() / '.platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-nm'

def read(elf, output):
    symbols = {}
    for line in subprocess.check_output([str(NM), '-g', str(elf)], text=True).splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2] in ('scalesDiagnostics', 'uwTick'):
            symbols[parts[2]] = int(parts[0], 16)
    if len(symbols) != 2:
        raise RuntimeError('ELF must match a build containing scalesDiagnostics and uwTick')
    with tempfile.TemporaryDirectory(prefix='blackpill-scale-read-') as temp:
        snapshot, tick = Path(temp)/'scales.bin', Path(temp)/'tick.bin'
        result = subprocess.run([CLI, '-c', 'port=SWD', 'mode=HOTPLUG', 'freq=1000',
            '-u', hex(symbols['scalesDiagnostics']), '32', str(snapshot),
            '-u', hex(symbols['uwTick']), '4', str(tick)],
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        Path(str(output)+'.log').write_text(result.stdout)
        if result.returncode or not snapshot.exists() or not tick.exists():
            raise RuntimeError('SWD read failed; see '+str(output)+'.log')
        raw1, raw2, spread, samples, sampled, loop_time, gross, status = struct.unpack('<iifIIIfI', snapshot.read_bytes())
        now = struct.unpack('<I', tick.read_bytes())[0]
    age = (now - sampled) & 0xffffffff
    report = dict(rawMean=[raw1, raw2], spreadG=spread if math.isfinite(spread) else None,
                  samples=samples, sampleAgeMs=age, controllerMillis=now,
                  grossWeightG=gross if math.isfinite(gross) else None,
                  trayStatus=('Not initialized' if loop_time == 0 and samples == 0 else
                              ['Present','Missing','Unavailable','Unconfigured','Settling'][status] if status < 5 else 'Unknown'),
                  settledFresh=samples == 4 and age <= 300 and math.isfinite(spread) and spread <= 1)
    output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', type=Path, default=ROOT/'.pio/build/all-pcb-stlink/firmware.elf')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    read(args.elf, args.output)
