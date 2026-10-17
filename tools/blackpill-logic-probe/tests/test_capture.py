import importlib.util
import json
from pathlib import Path
import unittest
from unittest import mock

spec = importlib.util.spec_from_file_location(
    "probe", Path(__file__).resolve().parents[1] / "probe.py")
probe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)


class CaptureTests(unittest.TestCase):
    def setUp(self):
        self.meta = dict(version=1, hz=100000000, duration_ticks=500,
                         max_poll_gap_ticks=20, truncated=False, count=8)
        self.lines = ["0,0", "100,1", "150,3", "200,2", "250,6",
                      "300,4", "400,0", "500,0"]

    def parse(self, lines=None):
        return probe.parse_capture("BEGIN " + json.dumps(self.meta),
                                   self.lines if lines is None else lines)

    def test_three_channels_with_different_high_intervals(self):
        stats = self.parse()["channels"]
        self.assertEqual([(c["rises"], c["falls"], c["high_percent"])
                          for c in stats], [(1, 1, 20), (1, 1, 30), (1, 1, 30)])

    def test_constant_high_is_not_an_edge(self):
        self.meta["count"] = 2
        stats = self.parse(["0,7", "500,7"])["channels"]
        self.assertTrue(all(c["rises"] == c["falls"] == 0 for c in stats))
        self.assertTrue(all(c["high_percent"] == 100 for c in stats))

    def test_invalid_or_incomplete_usb_capture_is_rejected(self):
        bad_cases = [self.lines[:-1], ["0,0", "100,8"] + self.lines[2:],
                     ["0,0", "151,1", "150,3"] + self.lines[3:],
                     self.lines[:-1] + ["499,0"]]
        for lines in bad_cases:
            with self.subTest(lines=lines), self.assertRaises(ValueError):
                self.parse(lines)

    def test_partial_buffer_capture_keeps_truncation_warning(self):
        self.meta["truncated"] = True
        self.assertTrue(self.parse()["truncated"])

    def test_usb_power_cycle_reopens_and_reidentifies_before_retry(self):
        device = probe.Probe.__new__(probe.Probe)
        disconnected = probe.SERIAL_IO_ERRORS[-1](6, "Device not configured")
        device._capture = mock.Mock(side_effect=[disconnected, {"capture": "ok"}])
        device.connect = mock.Mock()
        self.assertEqual(device.capture(250), {"capture": "ok"})
        device.connect.assert_called_once_with()
        self.assertEqual(device._capture.call_args_list,
                         [mock.call(250), mock.call(250)])


if __name__ == "__main__":
    unittest.main()
