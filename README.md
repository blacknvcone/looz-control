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
| [wiring.svg](wiring.svg) | Diagram wiring detail (buka di browser) |

## Pengujian (3 tahap, sebelum nyambung ke head unit)

1. **Input only** — KY-040 ke NodeMCU, verifikasi gesture via log serial (115200).
2. **Bench simulator** — jumper node KEY → A0, verifikasi level voltase &
   tuner (`k1/k2 <duty>`, `emit`, `test dbl`, `adc`).
3. **Head unit asli** — ukur pull-up dulu (3.3/5V lanjut, 12V stop), verifikasi
   idle = rail, lalu SWC learning + kalibrasi duty per tombol.

## Status

- [x] Rencana & desain hardware (PLAN/HARDWARE/DEBUG/wiring.svg)
- [ ] Firmware (step 1: scaffold `config.h` + sketch)
- [ ] Verifikasi Tahap 1–2 di meja
- [ ] Kalibrasi final di head unit (Tahap 3)

## Build & flash

Menyusul — toolchain (arduino-cli + ESP8266 core) belum terpasang di mesin
dev; command build/flash akan didokumentasikan di sini setelah scaffold
firmware selesai dan ter-compile.
