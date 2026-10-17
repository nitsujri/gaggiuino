import importlib.util
import json
from pathlib import Path
import struct
import unittest

spec = importlib.util.spec_from_file_location(
    "probe", Path(__file__).resolve().parents[1] / "probe.py")
probe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)


class AnalogTests(unittest.TestCase):
    def setUp(self):
        self.meta = dict(version=1, kind="analog", channel="PA0", encoding="u16le",
                         bits=6, count=256, adc_hz=36000000, cycles_per_sample=9,
                         vdda_mv=3300, vref_calibrated=True,
                         cpu_hz=72000000, elapsed_ticks=4608)
        self.values = list(range(64)) * 4

    def header(self):
        return "ANALOG " + json.dumps(self.meta)

    def payload(self):
        return struct.pack("<256H", *self.values)

    def test_fastest_voltage_scaling_and_sample_rate(self):
        result = probe.parse_analog(self.header(), self.payload())
        self.assertEqual(result["sample_hz"], 4000000)
        self.assertAlmostEqual(result["min_v"], 0)
        self.assertAlmostEqual(result["max_v"], 3.3)
        self.assertAlmostEqual(result["mean_v"], 1.65)

    def test_12bit_detail_and_long_acquisition_rates(self):
        self.meta.update(bits=12, cycles_per_sample=15)
        self.values = [0, 4095] * 128
        self.assertEqual(probe.parse_analog(self.header(), self.payload())["sample_hz"], 2400000)
        self.meta["cycles_per_sample"] = 492
        self.assertAlmostEqual(probe.parse_analog(self.header(), self.payload())["sample_hz"],
                               73170.731707, places=5)

    def test_truncated_and_out_of_range_samples_are_rejected(self):
        with self.assertRaises(ValueError):
            probe.parse_analog(self.header(), self.payload()[:-1])
        self.values[0] = 64
        with self.assertRaises(ValueError):
            probe.parse_analog(self.header(), self.payload())
        self.meta["count"] = 10000000
        with self.assertRaises(ValueError):
            probe.analog_metadata(self.header())

    def test_binary_samples_with_newlines_and_fragmented_usb_reads(self):
        self.meta.update(bits=12, cycles_per_sample=15)
        self.values = [0x0a0d, 0x0d0a] * 128
        wire = (self.header() + "\r\n").encode() + self.payload() + b"\r\nEND\r\n"

        class Serial:
            def __init__(self):
                self.pos = 0
                self.commands = []

            def reset_input_buffer(self):
                pass

            def write(self, data):
                self.commands.append(data)

            def readline(self):
                end = wire.index(b"\n", self.pos) + 1
                result = wire[self.pos:end]
                self.pos = end
                return result

            def read(self, count):
                end = self.pos + min(count, 3)
                result = wire[self.pos:end]
                self.pos = end
                return result

        device = probe.Probe.__new__(probe.Probe)
        device.serial = Serial()
        device.identity = "BLACKPILL_LOGIC_PROBE 1 analog=1"
        result = device.capture(250, "analog", 1, 256)
        self.assertEqual(result["samples"], self.values)
        self.assertEqual(device.serial.commands, [b"analog 1 256\n"])


if __name__ == "__main__":
    unittest.main()
