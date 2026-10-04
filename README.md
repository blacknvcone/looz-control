# looz-control

Control knob (rotary encoder) untuk **head unit Android** — NodeMCU/ESP8266
memalsukan tekanan tombol SWC (steering wheel control) pada garis **KEY1/KEY2**
menguji resistive ladder, dipelajari lewat mode SWC learning head unit.

## Gesture

| Gesture | Aksi |
|---|---|
| Putar kiri | Volume turun (VOL-) |
| Putar kanan | Volume naik (VOL+) |
| 1 tekan (lepas) | Play / Pause (bisa diganti MUTE) |
| 2 tekan | Next track |
| 3 tekan | Previous track |
| Long press ≥ 500 ms | Sleep head unit |

Klik terdaftar saat tombol **dilepas** (jendela multi-click ~400 ms);
long-press membatalkan klik tertunda. Semua timing di `config.h`.

## Hardware

Sistem final: **2 modul + 4 komponen inline** (tanpa PCB, tanpa modul output):

- NodeMCU v3 (ESP8266)
- KY-040 (EC11 20 detent + push switch, pull-up onboard)
- 2× 4.7 kΩ + 2× 100 nF — disisipkan inline di kabel dupont

**Cara kerja (singkat):**

- **Emit** — GPIO drive PWM 20 kHz → filter RC → tegangan DC yang dibentuk
  bersama pull-up internal head unit. Tiap slot tombol = satu level tegangan
  (duty 0–1000 dikalibrasi empiris saat SWC learning).
- **Idle** — GPIO mode INPUT (Hi-Z) → node terangkat penuh ke rail pull-up =
  kondisi "tanpa tombol" yang sempurna, berapa pun rail-nya (3.3V/5V).

Pinout: D5=CLK, D6=DT, D7=SW, D1=KEY1, D2=KEY2, A0=loopback debug.

## Dokumentasi

| Dokumen | Isi |
|---|---|
| [HARDWARE.md](HARDWARE.md) | Pemilihan komponen, wiring detail, prosedur SWC learning & kalibrasi |
| [DEBUG.md](DEBUG.md) | Simulator bench 3 tahap, serial console, checklist verifikasi |
| [PLAN.md](PLAN.md) | Rencana arsitektur firmware & urutan pengerjaan |
| [HID-SCREENSAVER-RESEARCH.md](HID-SCREENSAVER-RESEARCH.md) | Riset makro USB HID untuk membuka app screensaver |
| [wiring.svg](wiring.svg) | Diagram wiring detail (buka di browser) |

## Pengujian (3 tahap, sebelum nyambung ke head unit)

1. **Input only** — KY-040 ke NodeMCU, verifikasi gesture via log serial (115200).
2. **Bench simulator** — jumper node KEY → A0, verifikasi level voltase &
   tuner (`k1/k2 <duty>`, `emit`, `test dbl`, `adc`).
3. **Head unit asli** — ukur pull-up dulu (3.3/5V lanjut, 12V stop), verifikasi
   idle = rail, lalu SWC learning + kalibrasi duty per tombol.

## Status

- [x] Rencana & desain hardware (PLAN/HARDWARE/DEBUG/wiring.svg)
- [x] Toolchain: arduino-cli 1.5.1 + esp8266 core 3.1.2 (WSL2, via usbipd-win)
- [x] Firmware scaffold `config.h` + `looz-control.ino` — compile & flash OK
- [x] Console terverifikasi di board (banner, `adc`, `test`, `idle`, emit→Hi-Z)
- [x] LED indikator gesture (D4): blink N = nomor slot, per emit
- [x] Bug "putar kiri sesekali VOL+" — terbukti dari trace (DEBUG.md
      "Temuan bug arah"), decoder diganti quadrature state machine
      (CHANGE di CLK+DT, transisi 1-bit sah, ±4 = 1 detent)
- [x] False-press & detent terminim — coupling node `+` modul (VCC dibiarkan
      terbuka) terbukti via trace `SWC`; fix: pin `+` → 3V3 (HARDWARE.md §3.1)
- [x] Verifikasi setelah jumper `+` → 3V3 — LULUS (`traces/2026-10-03-verifikasi3.log`):
      blok arah bersih (kiri 20 CCW / kanan 13 CW / cepat 14 CCW), tanpa VOL+
      nyasar, `SWC` hanya saat tekan sungguhan, 47 detent utuh, drop 0
- [x] Verifikasi Tahap 1 (gesture fisik) — arah, klik, long-press, ramp cepat
- [ ] Verifikasi Tahap 2 (bench: loopback A0, tuner `k1`/`k2`)
- [ ] Kalibrasi final di head unit (Tahap 3)

## Build & flash (WSL2 + usbipd-win)

Port COM tidak terlihat dari WSL2 — NodeMCU di-pass-through dulu
(sekali `bind` di Admin PowerShell, `attach` tiap colok):

    usbipd bind --busid <ID>
    usbipd attach --wsl --busid <ID> --auto-attach

    # sekali saja: izin serial
    sudo usermod -aG dialout $USER

    # build & flash (telah diverifikasi)
    export PATH=$HOME/.local/bin:$PATH
    arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 .
    sg dialout -c "arduino-cli upload -p /dev/ttyUSB0 --fqbn esp8266:esp8266:nodemcuv2 ."

Console serial (banner boot + kirim perintah, terverifikasi):

    sg dialout -c "python3 tools/serial_probe.py"                 # banner saja
    sg dialout -c "python3 tools/serial_probe.py map adc"         # kirim perintah

Untuk sesi interaktif: `arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200`,
lalu tekan tombol **RST** di NodeMCU supaya banner ikut tercetak.
