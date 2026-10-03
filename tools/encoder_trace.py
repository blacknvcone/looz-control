#!/usr/bin/env python3
"""Rekam trace encoder mentah: reset board, set 'log 3', tangkap N detik.

Pemakaian (dari root repo):
  sg dialout -c "python3 tools/encoder_trace.py 60"              # 60 detik ke stdout
  sg dialout -c "python3 tools/encoder_trace.py 120 hasil.log"    # + simpan ke file

Catatan: buka port = NodeMCU ikut reset (auto-reset CP2102), jadi SEMUA
rekaman selalu dimulai dari boot bersih. Baris boot ROM (74880 baud) dibuang.
"""
import serial, time, sys

PORT = "/dev/ttyUSB0"
BAUD = 115200

secs = float(sys.argv[1]) if len(sys.argv) > 1 else 60.0
outfile = sys.argv[2] if len(sys.argv) > 2 else None

s = serial.Serial(PORT, BAUD, timeout=0.2)
time.sleep(1.5)                     # tunggu boot + banner
s.reset_input_buffer()
s.write(b"log 3\r\n")
time.sleep(0.3)

lines = []


def emit(text, ts):
    out = f"[{ts:7.2f}] {text}"
    print(out, flush=True)
    lines.append(out)
    if outfile:
        with open(outfile, "a") as f:
            f.write(out + "\n")


emit("=== capture mulai — PUTAR KNOB SEKARANG ===", 0.0)
t0 = time.time()
buf = b""
while time.time() - t0 < secs:
    d = s.read(512)
    if d:
        buf += d
        while b"\n" in buf:
            raw, buf = buf.split(b"\n", 1)
            text = raw.decode("utf-8", "replace").rstrip("\r")
            # buang boot ROM garbage (bukan ASCII bersih)
            if text and "�" not in text:
                emit(text, time.time() - t0)

s.write(b"log 2\r\n")               # kembalikan verbosity
time.sleep(0.2)
s.close()
emit("=== capture selesai ===", secs)
