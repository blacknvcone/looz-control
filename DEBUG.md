# looz-control — Debug & Simulator (tanpa head unit)

Tujuan: semua input (encoder, gesture) dan output (tegangan SWC) bisa diverifikasi
di meja sebelum satu kabel pun disentuh ke head unit Android.

Hardware final: **NodeMCU (ESP8266) + KY-040** — output PWM + RC inline
(2× 4.7 kΩ + 2× 100 nF), tanpa modul output.

---

## Arsitektur debug: 3 tahap

```
Tahap 1 — INPUT ONLY       Tahap 2 — BENCH SIMULATOR     Tahap 3 — REAL HEAD UNIT
┌────────────────────┐      ┌──────────────────────┐      ┌──────────────────────┐
│ KY-040 → log serial│      │ + jumper node KEY→A0 │      │ + node KEY → harness │
│ (belum ada output) │ ──►  │ (loopback ukur PWM)  │ ──►  │ + SWC learning mode │
│ Verifikasi gesture │      │ Verifikasi pipeline  │      │ Kalibrasi final      │
└────────────────────┘      └──────────────────────┘      └──────────────────────┘
```

| Tahap | Wiring | Yang dibuktikan |
|---|---|---|
| 1 | Hanya KY-040 ke NodeMCU | Encoder arah/putaran, klik 1/2/3, long-press, timing |
| 2 | + jumper loopback node KEY → A0 | PWM→tegangan benar, tiap slot beda level, tuner bekerja |
| 3 | + node KEY ke KEY1/KEY2 harness | Pull-up asli, learning mode menangkap, kalibrasi final |

---

## Tahap 1 — Event log input

Semua kejadian dicetak ke serial (115200 baud) dengan timestamp `millis()`:

```
[  12345] ENC   dir=CW  detent=3          ← putaran kanan, detent ke-3
[  12345] ENC   dir=CCW detent=1          ← putaran kiri
[  13010] BTN   press                     ← tekan
[  13620] BTN   release clicks=1          ← lepas, 1 klik
[  14050] BTN   release clicks=2 -> NEXT  ← jendela multi-click selesai, hasil NEXT
[  20010] BTN   longpress >=500ms -> SLEEP
```

Yang diuji di tahap ini (output & head unit mati):
- [x] Putaran kanan/kiri terdeteksi benar, tidak balik arah
- [x] Tidak ada detent ganda (noise) — kalau ada, naikkan debounce di config
- [x] Klik 1/2/3 terhitung benar, jendela 400 ms terasa pas
- [x] Long-press 500 ms tidak terbaca sebagai klik
- [x] Putaran cepat (volume ramp) tidak membanjiri log/event

Bukti: `traces/2026-10-03-verifikasi3.log` — blok arah bersih
(`0×16 CCW → 8 CW → 2 CCW` per detent, net −21 detent = −84 quarter persis),
ACT `VOL+` hanya pada blok putaran kanan, `SWC` hanya muncul saat tekan
sungguhan, buffer trace drop 0.

---

## Tahap 2 — Bench simulator

### Batasan yang HARUS dipahami

Desain PWM+RC memakai **pull-up head unit sebagai bagian divider**. Di bench
(tanpa head unit) pull-up itu tidak ada, jadi:

- **Emit di bench**: node = duty × 3.3V → span ≈ 0–3.3V.
- **Emit di mobil**: node = antara `[rail×Rser/(Rpull+Rser)]` dan `3.3V`
  (tergantung rail & Rpull) → span berbeda dari bench.
- Konsekuensi: **bench membuktikan pipeline & distinctness (relatif),
  kalibrasi absolut HANYA di Tahap 3** (SWC learning + tuner). Angka duty
  bench tidak dipakai final.

Opsional: pasang **dummy pull-up 10 kΩ → 3V3** di tiap node saat bench →
span bench ≈ 1.05–3.3V, lebih mirip kondisi asli (tapi angka final tetap
diambil di Tahap 3).

### Wiring bench

```
node KEY1 ──[jumper J1]──► A0     (ujung ke harness: TIDAK dipasang saat bench)
node KEY2 ──[jumper J2]──► A0     (ujung ke harness: TIDAK dipasang saat bench)
```

Catatan:
- **Jumper ke A0 hanya SATU garis dalam satu waktu** — J1+J2 bareng =
  KEY1 dan KEY2 saling pendek. Alternatif: DIP switch / toggle.
- Idle di bench = pin OUTPUT dilepas (Hi-Z) → node mengambang; A0 menariknya
  lewat divider internal (320k) → terbaca ≈0V. Ini artefak bench saja —
  di mobil (A0 dilepas) idle = rail persis.
- Emit bench maks ≈3.3V — masih aman di batas A0 (rating ≈3.2V, jangan
  dibiarkan lama di nilai maks).

### Verifikasi output di tahap ini

```
> emit vol+ 1000        ← fire slot manual, tahan 1000 ms
[  45010] ACT   slot=VOL+ line=KEY1 duty=600 meas=1.98V emit 1000ms
> adc
ADC   KEY1=1.98V KEY2=0.00V
> k1 750                ← tuner: ubah duty KEY1 live tanpa reflash
OK    KEY1 duty=750 -> meas=2.48V
> test dbl
[  51200] ACT   slot=NEXT line=KEY2 duty=400 meas=1.32V emit 100ms
> idle
IDLE  KEY1=Hi-Z KEY2=Hi-Z
```

Checklist tahap 2:
- [ ] `adc` cocok dengan multimeter (skala A0 terkalibrasi)
- [ ] Tiap slot menghasilkan voltase **berbeda** dari slot lain (jarak ≥ 0.3V)
- [ ] Saat boot / `idle`, pin output Hi-Z — node terbaca ≈0V di bench
      (artefak loading A0; yang penting pin benar-benar lepas)
- [ ] Emit selesai → kembali ke Hi-Z (release bersih)
- [ ] `test dbl`, `test long` → pipeline gesture→output jalan end-to-end
- [ ] Putaran volume: pulse per detent + repeat-rate terlihat benar di log
- [ ] Jarak antar slot duty minimal ~150 (dari skala 0–1000)

### Kalibrasi skala A0 (sekali saja)

1. `emit vol+` (biarkan menahan), ukur voltase node KEY1 dengan multimeter → mis. 1.98 V
2. Kirim `adc` di serial → baca nilai raw → mis. 610
3. Isi `ADC_SCALE = 1.98 / 610.0` di `config.h`, reflash
4. Sekarang `adc` menampilkan voltase yang ≈ sama dengan multimeter

---

## Tahap 3 — Real head unit (ringkas)

1. Lepas jumper loopback A0. Sambungkan **node KEY1 → KEY1** dan
   **node KEY2 → KEY2** di harness. **GND common wajib nyambung.**
2. Ukur dulu tegangan pull-up mengambang di pin KEY (multimeter, HARDWARE.md
   §3.3): 3.3V / 5V → lanjut; **12V → stop, lapor ke plan ini.**
3. **Verifikasi idle**: firmware idle (pin Hi-Z), ukur node → harus membaca
   rail persis (mis. 5.0V) dan head unit tidak mendeteksi tombol apa pun.
4. SWC learning mode di head unit → untuk tiap tombol layar, pakai serial
   tuner (`k1 <duty>` / `k2 <duty>`) sampai level terdeteksi stabil & berbeda.
5. Catat duty final ke `config.h`, reflash, verifikasi semua gesture.

---

## Serial console (semua tahap)

| Perintah | Fungsi |
|---|---|
| `help` | daftar perintah |
| `log 0..3` | verbosity log (0=mati, 1=action saja, 2=input+action, 3=debug) |
| `adc` | baca voltase loopback KEY1/KEY2 dari A0 |
| `k1 <duty>` / `k2 <duty>` | tuner duty PWM live, skala 0–1000 (contoh `k1 600`) |
| `emit <slot> [ms]` | fire satu slot manual (`emit prev 500`) |
| `test <gesture>` | injeksi gesture palsu, uji pipeline penuh (`test dbl`, `test long`, `test vol-`) |
| `map` | tampilkan tabel gesture → slot → duty |
| `idle` | lepas semua output (pin → Hi-Z) |

Slot: `vol- vol+ play mute next prev sleep` (sesuai config.h)

---

## LED indikator gesture (D4 onboard)

Setiap emit sukses (gesture benar-benar di-drive ke KEY), LED onboard D4
blink **N kali, N = nomor slot** — non-blocking, tidak mengganggu loop:

| blink | slot | gesture |
|---|---|---|
| 1 | VOL- | putar kiri |
| 2 | VOL+ | putar kanan |
| 3 | PLAY | 1 tekan |
| 4 | MUTE | (slot cadangan, belum dipetakan gesture) |
| 5 | NEXT | 2 tekan |
| 6 | PREV | 3 tekan |
| 7 | SLEEP | long press ≥ 500 ms |

Fase: 70 ms nyala / 70 ms mati (`LED_ON_MS` / `LED_OFF_MS` di `config.h`).
Saat volume ramp (repeat 120 ms) pola restart tiap pulse → LED berkedip
terus; arah per detent tetap dibaca dari log `ENC`/`ACT`.

Catatan: saat `hold` tuner (`k1`/`k2`) LED **tidak** blink — hold bukan gesture.

---

## Trace encoder mentah (log level 3)

Untuk membedakan "software salah decode" vs "hardware/kalibrasi", pakai trace
per-edge — satu baris untuk setiap edge yang sampai ke ISR:

```
> log 3
[  12345] TRC   t=918273us gap=4310 dir=CW  CLK=1 DT=0 -> diterima
[  12345] TRC   t=918310us gap=37   dir=CW  CLK=1 DT=0 -> tolak debounce
[  12345] ENC   dir=CCW detent=4
```

Arti field:

| field | makna |
|---|---|
| `t` | `micros()` saat ISR berjalan |
| `gap` | µs sejak edge terakhir yang **diproses** (debounce `ENC_DEBOUNCE_US`=800 menolak `gap < 800`; edge ditolak tidak me-reset jendela) |
| `dir` | arah transisi menurut state machine: CW / CCW / `--` (transisi tidak sah) |
| `CLK`/`DT` | level pin saat ISR berjalan |
| verdict | `diterima` / `tolak debounce` / `transisi tidak sah` (2 bit berubah = ISR telat / noise) |

Decoder sekarang: **interrupt CHANGE di CLK *dan* DT** + state machine —
transisi sah hanya bila tepat satu bit berubah; bounce (transisi + balikan)
saling cancel; akumulasi 1/4 detent, ±4 = 1 detent. **State `encPrev` SELALU
di-update di tiap edge** — hanya *penghitungan* yang diberi jendela
`ENC_DEBOUNCE_US`. (Versi pertama tidak meng-update state saat jendela →
transisi nyata < jendela terbaca lompat 2-bit → detent hilang; jangan ulangi.)

Log level 3 juga mencetak perubahan level switch, untuk telusur false-press:

```
[  38010] SWC   raw=0 CLK=1 DT=1     ← SW jadi rendah: saat kedua line encoder tinggi?
                                        = bukan coupling kontak → kabel/noise
[  38040] SWC   raw=1 CLK=0 DT=1     ← SW lepas saat kontak tertutup → suspect
                                        coupling node internal modul
```

### Cara uji arah (Tahap 1)

1. `log 3`, putar **kiri** pelan 5–10 detent, catat output (→ `tools/serial_capture.py`).
2. Putar **kanan** pelan, catat lagi.
3. Periksa: selama putaran kiri, **tidak boleh ada** baris
   `dir=CW ... -> diterima`. Putaran kanan sebaliknya.

Kalau ketemu `dir=CW -> diterima` di tengah putaran kiri, arah memang salah
decode (lanjut ke analisis bounce di bawah) — bukan salah kalibrasi duty.

### Temuan bug arah — TERBUKTI dari trace (2026-10-03)

Rekaman: `traces/2026-10-03-arah2.log` (666 baris, `log 3`, 99 detent —
87 CCW / 12 CW; user melaporkan LED kedip 2× saat putar kiri = VOL+ nyasar).

Bukti numerik dari rekaman itu:

1. **14 edge `dir=CW -> diterima`** — semuanya `CLK=1 DT=0`, semuanya diikuti
   cluster bounce (`tolak debounce`) 8–500 µs kemudian, dan `gap`-nya besar
   (2.3 ms – 3.4 detik = hanya mungkin saat putaran **lambat**).
   Contoh paling jelas: `gap=114914 dir=CW CLK=1 DT=0 -> diterima`
   langsung diikuti `ACT slot=VOL+`, dan 80 ms kemudian `ENC dir=CCW`.
2. **Edge yang ditolak berbanding 145 CW : 16 CCW**, padahal sesi 89% putaran
   kiri. Asimetri ini = sidik jari bounce kontak CLK di transisi **jatuh**nya
   saat putaran kiri: DT sudah lebih dulu rendah → edge pantul terbaca `CW`.
3. Mekanisme: decoder lama hanya memakai **edge naik CLK**. Edge pantul itu
   yang diterima (karena `gap` besar), edge asli berikutnya jatuh di dalam
   jendela 1500 µs → dibuang → satu detent tercatat **+1 arah salah** →
   `volAccum` perlahan positif → VOL+ terkirim.

Bug ikutan yang ikut diperbaiki: `encDetentTotal` unsigned → log
`detent=4294967293` (wrap saat net CCW); sekarang `int32_t` / `%ld`.

**Perbaikan:** decoder diganti quadrature state machine — interrupt `CHANGE`
di CLK *dan* DT, transisi sah hanya bila tepat satu bit berubah (bounce
saling cancel, transisi 2-bit diabaikan), debounce 1500 µs pada penghitungan
sambil state tetap di-update, akumulasi 1/4 detent (±4 = 1 detent).

### Temuan 2 — coupling node internal modul (false-press + detent terminim)

Rekaman: `traces/2026-10-03-verifikasi2.log` (log 3, termasuk jejak `SWC`).

1. **22/23 perubahan level SW selalu seiring state CLK/DT** — pola
   `SWC raw=1 CLK=0 DT=0` / `raw=0 CLK=1 DT=1` (raw=1 = pin SW rendah).
   Tiga line bergerak **bareng, arah sama** → bukan noise acak: ketiganya
   tersambung ke node yang sama.
2. Mekanisme: pull-up modul direferensikan ke pin `+` yang **dibiarkan
   terbuka** (keputusan lama di HARDWARE.md §3.1). Saat kontak encoder
   menutup (line → GND), node `+` ikut tertarik turun → dua line lain anjlok
   ke ~1–1,7 V = ambang input ESP8266. Akibatnya:
   - **False press**: SW terbaca LOW ≥30 ms selama kontak menutup → 58 siklus
     press/release palsu di sesi rekaman 1 (user tidak pernah menekan), sempat
     ke-trigger longpress → SLEEP.
   - **Detent terminim**: dua bit tampak berubah serempak → 84% event
     `transisi tidak sah` → quadrature tak terbaca → cuma 2 detent dari
     puluhan yang diputar.
   - Bug arah decoder lama ikut terjelaskan: DT disample tepat di ambang.
3. **Perbaikan: jumper pin `+` modul → 3V3 NodeMCU** (bukan 5V — GPIO ESP8266
   bukan 5V-toleran; HARDWARE.md §3.1 sudah dibetulkan).

Kriteria lulus verifikasi setelah jumper: `SWC` hanya muncul saat menekan
sungguhan, `transisi tidak sah` turun drastis, jumlah `ENC` kembali setara
jumlah putaran, `ACT` tidak pernah `VOL+` saat putaran kiri.

**Status: LULUS** — `traces/2026-10-03-verifikasi3.log` (setelah jumper
`+` → 3V3): 0 false-press (`SWC` hanya 4 perubahan = 2 tekanan sungguhan,
semua saat `CLK=1 DT=1`), blok arah bersih tanpa arah nyasar, 47 detent dari
198 quarter (net −84 quarter = −21 detent, konsisten), drop 0. Sisa
`transisi tidak sah` ±53% = bounce yang terbaca diagonal <100 µs — tidak
merusak hitungan (tiap detent tetap utuh); pengerasan opsional: RC hardware
di CLK/DT.

---

## Referensi

- Wiring hardware & prosedur SWC learning: [HARDWARE.md](HARDWARE.md)
- Rencana firmware & urutan build: [PLAN.md](PLAN.md)
