#!/usr/bin/env python3
"""USB capture and localhost dashboard for the separate BlackPill logic probe."""
import argparse
import json
import time
import struct
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
from urllib.parse import urlparse

try:
    import termios
    SERIAL_IO_ERRORS = (OSError, termios.error)
except ImportError:
    SERIAL_IO_ERRORS = (OSError,)


def parse_capture(header, lines):
    if not header.startswith("BEGIN "):
        raise ValueError("Missing capture header")
    meta = json.loads(header[6:])
    events = [list(map(int, line.split(","))) for line in lines]
    if meta.get("version") != 1 or meta.get("hz", 0) <= 0:
        raise ValueError("Unsupported capture format")
    if len(events) != meta["count"] or len(events) < 2:
        raise ValueError("Incomplete capture")
    previous = -1
    for tick, state in events:
        if tick < previous or not 0 <= state <= 7:
            raise ValueError("Invalid event")
        previous = tick
    if events[0][0] != 0 or events[-1][0] != meta["duration_ticks"]:
        raise ValueError("Invalid capture endpoints")
    meta["events"] = events
    stats = []
    for bit, name in enumerate(("CLK · PA0", "D1 · PA1", "D2 · PA2")):
        rises = falls = high_ticks = 0
        for (t0, s0), (t1, s1) in zip(events, events[1:]):
            high_ticks += (t1 - t0) if s0 & (1 << bit) else 0
            if (s0 ^ s1) & (1 << bit):
                if s1 & (1 << bit):
                    rises += 1
                else:
                    falls += 1
        stats.append(dict(name=name, rises=rises, falls=falls,
                          initial=(events[0][1] >> bit) & 1,
                          final=(events[-1][1] >> bit) & 1,
                          high_percent=100 * high_ticks / max(1, meta["duration_ticks"])))
    meta["channels"] = stats
    return meta


def analog_metadata(header):
    if not header.startswith("ANALOG "):
        raise ValueError("Missing analog header")
    meta = json.loads(header[7:])
    if (meta.get("version") != 1 or meta.get("kind") != "analog" or
            meta.get("encoding") != "u16le" or meta.get("channel") != "PA0"):
        raise ValueError("Unsupported analog format")
    count = meta.get("count")
    bits = meta.get("bits")
    if type(count) is not int or not 256 <= count <= 32768 or bits not in (6, 12):
        raise ValueError("Invalid analog sample count/resolution")
    if not 0 < meta.get("adc_hz", 0) <= 36000000:
        raise ValueError("ADC clock outside supported range")
    if meta.get("cycles_per_sample") not in (bits + 3, bits + 480):
        raise ValueError("Invalid ADC conversion timing")
    if not 2400 <= meta.get("vdda_mv", 0) <= 3600:
        raise ValueError("Invalid voltage reference")
    return meta


def parse_analog(header, payload):
    meta = analog_metadata(header)
    if len(payload) != meta["count"] * 2:
        raise ValueError("Incomplete analog payload")
    samples = list(struct.unpack(f'<{meta["count"]}H', payload))
    full_scale = (1 << meta["bits"]) - 1
    if max(samples) > full_scale:
        raise ValueError("ADC samples exceed selected resolution")
    scale = meta["vdda_mv"] / 1000 / full_scale
    meta.update(samples=samples,
                sample_hz=meta["adc_hz"] / meta["cycles_per_sample"],
                min_v=min(samples) * scale, max_v=max(samples) * scale,
                mean_v=sum(samples) / len(samples) * scale,
                quantization_mv=meta["vdda_mv"] / full_scale)
    return meta


class Probe:
    def __init__(self, port):
        self.port = port
        self.serial = None
        self.connect()

    def connect(self):
        import serial
        if self.serial is not None:
            self.serial.close()
        self.serial = serial.Serial(self.port, 115200, timeout=0.3, write_timeout=2)
        time.sleep(0.3)
        self.serial.reset_input_buffer()
        self.serial.write(b"info\n")
        deadline = time.monotonic() + 4
        while time.monotonic() < deadline:
            line = self.line()
            if line.startswith("BLACKPILL_LOGIC_PROBE 1 "):
                if "timer=ok" not in line:
                    self.serial.close()
                    raise RuntimeError("Probe cycle counter failed its startup check")
                self.identity = line
                return
        self.serial.close()
        raise RuntimeError("That serial device did not identify as BLACKPILL_LOGIC_PROBE")

    def line(self):
        return self.serial.readline().decode("ascii", errors="replace").strip()

    def capture(self, ms, mode="digital", preset=0, count=32768):
        if mode not in ("digital", "analog"):
            raise ValueError("Unknown capture mode")
        if mode == "digital" and not 10 <= ms <= 1000:
            raise ValueError("Capture duration must be 10–1000 ms")
        if mode == "analog" and (preset not in (0, 1, 2) or not 256 <= count <= 32768):
            raise ValueError("Invalid analog preset/sample count")
        operation = (lambda: self._analog_capture(preset, count)) if mode == "analog" else (lambda: self._capture(ms))
        try:
            return operation()
        except SERIAL_IO_ERRORS:
            # A USB power cycle invalidates the open file descriptor even when
            # macOS assigns the same serial path. Reopen and identify it again.
            self.connect()
            return operation()

    def read_exact(self, count):
        payload = bytearray()
        deadline = time.monotonic() + 15
        while len(payload) < count and time.monotonic() < deadline:
            payload.extend(self.serial.read(count - len(payload)))
        if len(payload) != count:
            raise RuntimeError("Incomplete analog USB transfer")
        return bytes(payload)

    def _analog_capture(self, preset, count):
        if "analog=1" not in self.identity:
            raise RuntimeError("Analog mode requires the updated spare probe firmware")
        self.serial.reset_input_buffer()
        self.serial.write(f"analog {preset} {count}\n".encode())
        deadline = time.monotonic() + 15
        while time.monotonic() < deadline:
            header = self.line()
            if header.startswith("ERROR "):
                raise RuntimeError(header)
            if header.startswith("ANALOG "):
                meta = analog_metadata(header)
                if meta["count"] != count:
                    raise ValueError("Unexpected analog sample count")
                payload = self.read_exact(meta["count"] * 2)
                if self.read_exact(2) != b"\r\n" or self.line() != "END":
                    raise ValueError("Invalid analog frame terminator")
                return parse_analog(header, payload)
        raise RuntimeError("Analog capture timed out")

    def _capture(self, ms):
        self.serial.reset_input_buffer()
        self.serial.write(f"capture {ms}\n".encode())
        deadline = time.monotonic() + 15
        header = None
        lines = []
        while time.monotonic() < deadline:
            line = self.line()
            if not line:
                continue
            if line.startswith("ERROR "):
                raise RuntimeError(line)
            if header is None:
                if line.startswith("BEGIN "):
                    header = line
            elif line == "END":
                return parse_capture(header, lines)
            else:
                lines.append(line)
                if len(lines) > 8192:
                    raise RuntimeError("Unexpected capture size")
        raise RuntimeError("Capture timed out; reconnect the probe and restart this program")

    def close(self):
        self.serial.close()


def serve(probe, port, output):
    page = Path(__file__).with_name("dashboard.html")
    latest = None

    class Handler(BaseHTTPRequestHandler):
        def reply(self, code, value, mime="application/json"):
            body = value if isinstance(value, bytes) else json.dumps(value).encode()
            self.send_response(code)
            self.send_header("Content-Type", mime)
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            self.wfile.write(body)

        def do_GET(self):
            path = urlparse(self.path).path
            if path == "/":
                self.reply(200, page.read_bytes(), "text/html; charset=utf-8")
            elif path == "/api/capture":
                self.reply(200, dict(identity=probe.identity, capture=latest))
            else:
                self.reply(404, dict(error="Not found"))

        def do_POST(self):
            nonlocal latest
            if self.path != "/api/capture":
                self.reply(404, dict(error="Not found"))
                return
            origin = self.headers.get("Origin")
            if origin and origin != f"http://127.0.0.1:{port}":
                self.reply(403, dict(error="Local dashboard requests only"))
                return
            try:
                size = int(self.headers.get("Content-Length", 0))
                if not 0 < size <= 1024:
                    raise ValueError("Invalid request size")
                request = json.loads(self.rfile.read(size))
                latest = probe.capture(int(request.get("ms", 250)),
                                       mode=request.get("mode", "digital"),
                                       preset=int(request.get("preset", 0)),
                                       count=int(request.get("count", 32768)))
                if output:
                    output.write_text(json.dumps(latest, indent=2) + "\n")
                self.reply(200, dict(identity=probe.identity, capture=latest))
            except (ValueError, RuntimeError) + SERIAL_IO_ERRORS as exc:
                self.reply(400, dict(error=str(exc)))

        def log_message(self, *_args):
            pass

    server = HTTPServer(("127.0.0.1", port), Handler)
    print(f"Dashboard: http://127.0.0.1:{port}  (Ctrl+C to stop)", flush=True)
    try:
        server.serve_forever()
    finally:
        server.server_close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--list", action="store_true", help="List USB serial devices")
    parser.add_argument("--port", help="Serial path of the spare running probe firmware")
    parser.add_argument("--once", action="store_true", help="Capture once and print JSON")
    parser.add_argument("--ms", type=int, default=250)
    parser.add_argument("--mode", choices=("digital", "analog"), default="digital")
    parser.add_argument("--preset", type=int, choices=(0, 1, 2), default=0,
                        help="Analog: 0=fast 6-bit, 1=fast 12-bit, 2=long-sample 12-bit")
    parser.add_argument("--count", type=int, default=32768)
    parser.add_argument("--http-port", type=int, default=8765)
    parser.add_argument("--output", type=Path, help="Save latest capture JSON here")
    args = parser.parse_args()
    if args.list:
        from serial.tools import list_ports
        for item in list_ports.comports():
            print(f"{item.device}\t{item.description}\tserial={item.serial_number}")
        return
    if not args.port:
        parser.error("Specify --port explicitly; use --list to find the spare")
    if not 10 <= args.ms <= 1000:
        parser.error("--ms must be 10–1000")
    if not 1 <= args.http_port <= 65535:
        parser.error("--http-port must be 1–65535")
    probe = Probe(args.port)
    try:
        if args.once:
            result = probe.capture(args.ms, args.mode, args.preset, args.count)
            encoded = json.dumps(result, indent=2) + "\n"
            if args.output:
                args.output.write_text(encoded)
            print(encoded, end="")
        else:
            print(probe.identity, flush=True)
            serve(probe, args.http_port, args.output)
    finally:
        probe.close()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
