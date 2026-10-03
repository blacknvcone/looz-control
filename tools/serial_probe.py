#!/usr/bin/env python3
"""Reset NodeMCU via RTS/DTR, tampilkan banner boot, kirim perintah console.

Pemakaian (dari root repo):
  sg dialout -c "python3 tools/serial_probe.py"              # hanya banner
  sg dialout -c "python3 tools/serial_probe.py map adc"      # kirim perintah
  sg dialout -c "python3 tools/serial_probe.py 'test dbl'"   # quote bila ada spasi
"""
import serial, time, sys

PORT = "/dev/ttyUSB0"
BAUD = 115200

s = serial.Serial(PORT, BAUD, timeout=0.3)
s.dtr = False          # IO0 HIGH — boot normal, bukan bootloader
time.sleep(0.05)
s.rts = True           # EN LOW — chip di-reset
time.sleep(0.15)
s.rts = False          # EN HIGH — rilis → firmware boot
time.sleep(1.0)
s.reset_input_buffer()

def drain(sec=1.0):
    t0 = time.time()
    buf = b""
    while time.time() - t0 < sec:
        d = s.read(256)
        if d:
            buf += d
    return buf.decode("utf-8", "replace")

print(drain(2.0), end="")
for cmd in sys.argv[1:]:
    print(f">>> {cmd}")
    s.write((cmd + "\r\n").encode())
    print(drain(1.2), end="")
s.close()
