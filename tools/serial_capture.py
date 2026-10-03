#!/usr/bin/env python3
"""Tangkap log serial NodeMCU tanpa reset chip (monitor saja).

Pemakaian (dari root repo):
  sg dialout -c "python3 tools/serial_capture.py"             # 20 detik ke stdout
  sg dialout -c "python3 tools/serial_capture.py 60 out.log"  # 60 detik ke file

Tidak mengubah DTR/RTS (dtr=False, rts=False) supaya board tidak ikut
reset saat port dibuka — cocok untuk mengamati bug saat knob diputar.
"""
import serial, time, sys

PORT = "/dev/ttyUSB0"
BAUD = 115200

secs = float(sys.argv[1]) if len(sys.argv) > 1 else 20.0
outfile = sys.argv[2] if len(sys.argv) > 2 else None

s = serial.Serial()
s.port = PORT
s.baudrate = BAUD
s.timeout = 0.2
s.dtr = False
s.rts = False
s.open()
s.dtr = False
s.rts = False

t0 = time.time()
buf = b""
while time.time() - t0 < secs:
    d = s.read(512)
    if d:
        buf += d
        while b"\n" in buf:
            line, buf = buf.split(b"\n", 1)
            ts = time.time() - t0
            text = line.decode("utf-8", "replace").rstrip("\r")
            if text:
                out = f"[{ts:7.2f}] {text}"
                print(out, flush=True)
                if outfile:
                    with open(outfile, "a") as f:
                        f.write(out + "\n")
s.close()
