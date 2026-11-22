#!/usr/bin/env python3
"""Run the simulated node, drive it over its UART shell, and verify the telemetry stream.

Usage: python tools/uart_monitor.py path/to/zephyr.exe [--scenario]
"""
import argparse
import queue
import subprocess
import sys
import threading
import time

POLY = 0x1021


def crc16_itu_t(data: bytes, crc: int = 0) -> int:
    """Same CRC as Zephyr's crc16_itu_t (poly 0x1021, MSB first, init 0)."""
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ POLY) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


FIELDS = ["seq", "ms", "temp_mc", "ax", "ay", "az", "gx", "gy", "gz", "mx", "my", "mz", "alarms"]


def parse_frame(line: str):
    """Return dict, or None if not a frame. Raises ValueError on CRC/format error."""
    line = line.strip()
    if not line.startswith("$SN,"):
        return None
    body, _, crc = line[1:].rpartition("*")
    if crc16_itu_t(body.encode()) != int(crc, 16):
        raise ValueError(f"bad CRC: {line}")
    parts = body.split(",")[1:]
    if len(parts) != len(FIELDS):
        raise ValueError(f"bad field count: {line}")
    return dict(zip(FIELDS, map(int, parts)))


class Node:
    def __init__(self, exe):
        self.p = subprocess.Popen([exe], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT, bufsize=0)
        self.frames, self.bad, self.text = [], 0, []
        self.q = queue.Queue()
        threading.Thread(target=self._reader, daemon=True).start()

    def _reader(self):
        buf = b""
        while True:
            ch = self.p.stdout.read(1)
            if not ch:
                return
            if ch in (b"\n", b"\r"):
                if buf:
                    self.q.put(buf.decode(errors="replace"))
                buf = b""
            else:
                buf += ch

    def send(self, cmd):
        self.p.stdin.write(cmd.encode() + b"\r\n")
        self.p.stdin.flush()

    def pump(self, seconds):
        end = time.time() + seconds
        while time.time() < end:
            try:
                line = self.q.get(timeout=0.05)
            except queue.Empty:
                continue
            i = line.find("$SN,")
            try:
                f = parse_frame(line[i:]) if i >= 0 else None
            except ValueError:
                self.bad += 1
                continue
            if f:
                self.frames.append(f)
            else:
                self.text.append(line)

    def stop(self):
        self.p.terminate()


def check(cond, msg, results):
    results.append((cond, msg))
    print(("PASS " if cond else "FAIL ") + msg)


def scenario(exe):
    r = []
    n = Node(exe)
    n.pump(0.3)
    n.send("node period 100")
    n.pump(3)
    base = [f for f in n.frames if f["alarms"] == 0]
    check(len(n.frames) >= 20, f"streams frames at 100 ms ({len(n.frames)} in ~3 s)", r)
    check(n.bad == 0, "every frame has a valid CRC", r)
    seqs = [f["seq"] for f in n.frames]
    check(seqs == list(range(seqs[0], seqs[0] + len(seqs))), "sequence numbers are gap-free", r)
    check(len(base) == len(n.frames), "no alarms at rest", r)
    f = n.frames[-1]
    check(20000 < f["temp_mc"] < 35000, f"temperature plausible ({f['temp_mc']/1000:.2f} C)", r)
    check(900 < f["az"] < 1100, f"gravity on Z ~1000 mg ({f['az']})", r)

    n.frames.clear()
    n.send("node thresh_temp 40")
    n.send("sim heat 20")
    n.pump(3)
    check(any(f["alarms"] & 1 for f in n.frames), "over-temperature alarm trips", r)
    n.send("sim heat 0")
    n.pump(5)
    check(not (n.frames[-1]["alarms"] & 1), "over-temperature clears after cool-down (hysteresis)", r)

    n.frames.clear()
    n.send("sim shock 600")
    n.pump(2)
    check(any(f["alarms"] & 2 for f in n.frames), "shock alarm trips on impact", r)
    check(not (n.frames[-1]["alarms"] & 2), "shock alarm clears after impact", r)

    n.frames.clear()
    n.send("sim spin 800")
    n.pump(2)
    check(any(f["alarms"] & 4 for f in n.frames), "spin alarm trips on rotation", r)

    n.send("node status")
    n.pump(0.5)
    check(any("frames=" in t and "i2c_errors=0" in t for t in n.text), "shell status reports 0 I2C errors", r)
    n.stop()
    return all(c for c, _ in r)


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("exe")
    a = ap.parse_args()
    sys.exit(0 if scenario(a.exe) else 1)
