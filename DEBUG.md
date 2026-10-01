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
- [ ] Putaran kanan/kiri terdeteksi benar, tidak balik arah
- [ ] Tidak ada detent ganda (noise) — kalau ada, naikkan debounce di config
- [ ] Klik 1/2/3 terhitung benar, jendela 400 ms terasa pas
- [ ] Long-press 500 ms tidak terbaca sebagai klik
- [ ] Putaran cepat (volume ramp) tidak membanjiri log/event

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

## Referensi

- Wiring hardware & prosedur SWC learning: [HARDWARE.md](HARDWARE.md)
- Rencana firmware & urutan build: [PLAN.md](PLAN.md)
