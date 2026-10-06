# looz-control — Rencana

Control knob berbasis ESP8266 yang membaca rotary encoder + tombol dan meniru
tekanan tombol setir (SWC) pada garis **KEY1 / KEY2** head unit Android
(voltage-ladder, dipelajari lewat mode SWC learning head unit).

> **Target: head unit Android.** Detail komponen & wiring: [HARDWARE.md](HARDWARE.md).
> Test input/output sebelum nyambung ke head unit: [DEBUG.md](DEBUG.md).

## Keputusan

- **MCU: NodeMCU v3 (ESP8266)** ✅ — sudah dimiliki, cukup, tidak perlu RP2040.
  A0 (ADC tunggal) dipakai ulang sebagai loopback debug untuk mengukur tegangan
  output di bench.
- **Encoder: KY-040 module** ✅ — EC11 20 detent + push switch, pull-up sudah
  onboard. KEY1/KEY2 di harness head unit bisa diakses.
- **Output: PWM + RC langsung dari GPIO** ✅ (tanpa MCP4728) — 4 komponen
  inline di kabel dupont (2× 4.7 kΩ + 2× 100 nF). **Idle = Hi-Z (pin INPUT)**
  → node tegak penuh di rail pull-up = open sempurna untuk rail berapa pun
  (3.3/5/12V), nol risiko phantom. Sistem final = 2 modul (NodeMCU + KY-040).
- **Timing gesture: klik klasik saat lepas tombol** —
  - klik terdaftar saat tombol **dilepas**; hasil 1/2/3 klik diputuskan setelah
    jendela tunggu ~400 ms
  - long-press aktif saat tahan ≥ **500 ms** dan membatalkan klik yang tertunda
  - semua timing ada di `config.h`
- **Output SWC: voltage ladder** — head unit punya mode SWC learning; kita
  pancarkan satu tegangan DC khas per slot tombol lalu ajarkan ke head unit.

## Arsitektur

```
[Encoder A/B]──► input encoder (interrupt, debounce, hitung detent)
[Encoder key]──► input tombol (FSM press/release/long-press/multi-click)
                        │ event gesture (VOL_UP, VOL_DOWN, PLAY_PAUSE,
                        ▼                             NEXT, PREV, SLEEP)
                 action mapper (config: gesture → slot tombol)
                        │
                 driver output SWC ──► garis KEY1 / KEY2 (PWM+RC → level; idle=Hi-Z)
```

### 1. Lapisan input

- Encoder A/B lewat interrupt, decode kuadratur + debounce → event
  `rotate_left` / `rotate_right`.
- FSM tombol: klik (saat lepas, jendela multi-click), long-press ≥ 500 ms.

### 2. Pemetaan aksi (inti requirement "register some action")

Tabel di `config.h` — gesture → slot SWC:

| Gesture      | Aksi                           |
|--------------|--------------------------------|
| putar kiri   | VOL-                           |
| putar kanan  | VOL+                           |
| 1 tekan      | PLAY/PAUSE (bisa diganti MUTE) |
| 2 tekan      | NEXT                           |
| 3 tekan      | PREV                           |
| long press   | SLEEP                          |

- Volume memancarkan pulse pendek berulang selama diputar (dibatasi
  repeat-rate), bukan satu pulse per detent yang membanjiri.

### 3. Driver output SWC (emulasi voltage ladder)

- Tiap slot tombol = satu level tegangan di KEY1 atau KEY2.
- Emit: PWM 20 kHz + RC (4.7 kΩ/100 nF) → duty 0–1000 per slot. Level voltase
  final dikalibrasi **empiris** (serial tuner saat SWC learning) karena
  tergantung Rpull head unit — bukan hitungan teori.
- **Idle = pin INPUT (Hi-Z)** → node terangkat rail = open sempurna,
  rail-agnostic, nol phantom; pin D1/D2 juga Hi-Z alami saat boot.
- Distribusi slot (bisa disesuaikan — head unit biasanya membatasi jumlah tombol
  per garis, sering 3–6):
  - KEY1: VOL-, VOL+, PLAY_PAUSE
  - KEY2: NEXT, PREV, SLEEP
- Alternatif tahap output (resistor divider / optocoupler dry-contact)
  didokumentasikan di HARDWARE.md kalau input ternyata bukan resistive ladder.

## Hasil kerja (di `~/RavenProject/looz-control`)

- `looz-control/looz-control.ino` — sketch utama
- `looz-control/config.h` — pin, timing, peta gesture→slot, duty PWM per tombol
- `HARDWARE.md` — pemilihan komponen, wiring, prosedur SWC learning
- `DEBUG.md` — simulator bench, serial console, checklist verifikasi
- compile-verified dengan arduino-cli + ESP8266 core
  (**install hanya kalau ada izin eksplisit**)

## Urutan pengerjaan

1. Scaffold proyek + `config.h` (tanpa install, bisa direview)
2. Encoder + FSM tombol → logging serial (verifikasi gesture di meja) — **Tahap 1**
3. Driver output SWC (PWM+RC, Hi-Z idle) + bench simulator (loopback A0 +
   serial console) — **Tahap 2**, lihat [DEBUG.md](DEBUG.md)
4. Wiring ke head unit, jalankan SWC learning, tuning tegangan per tombol — **Tahap 3**
5. Soak test: volume ramp, klik 1/2/3, long-press sleep

## Catatan terbuka

1. Bentuk input KEY1/KEY2 head unit — kemungkinan besar resistive ladder ke ADC
   (umum di unit Android), tapi **wajib diukur dulu** sebelum nyambung: tegangan
   pull-up mengambang di pin KEY (lihat HARDWARE.md §3.3). Kalau ternyata 12V
   logika/pulse, desain output perlu disesuaikan.
2. Encoder & akses harness: ✅ sudah terjawab — KY-040, KEY1/KEY2 bisa diakses.
