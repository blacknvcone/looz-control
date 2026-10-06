# looz-control — Hardware & Wiring (Head Unit Android)

Panduan detail pemilihan komponen dan wiring untuk **NodeMCU (ESP8266) + KY-040**
sebagai pengganti tombol SWC (steering wheel control) di head unit Android.
Sistem final: **2 modul + 4 komponen inline (2× 4.7 kΩ + 2× 100 nF) di kabel
dupont — tanpa PCB, tanpa modul output.**

> Test input & output sebelum nyambung ke head unit → [DEBUG.md](DEBUG.md).

---

## 1. Pemilihan Encoder — kenapa KY-040?

KY-040 = modul EC11 yang sudah jadi di PCB. Karena KY-040 berbasis EC11,
varian EC11 tetap relevan kalau nanti mau upgrade ke bare. 5 hal yang
perlu diperhatikan:

### 1.1 Wajib: ada push switch ("with push button")

Setiap gesture tekan (1/2/3 klik & long-press) butuh switch pada poros encoder.
Varian EC11 ada 2 jenis:

| Varian | Pin | Untuk kita |
|---|---|---|
| **EC11 dengan push switch** | 5 pin: A, C, B (sisi encoder) + SW1, SW2 (switch) | ✅ **WAJIB** |
| EC11 tanpa switch | 3 pin: A, C, B saja | ❌ Tidak bisa tekan |

Ciri fisik: varian switch punya 2 pin tambahan di sisi lain dari pin A/C/B.

### 1.2 Detent (klik per putaran)

| Detent | Karakter | Rekomendasi |
|---|---|---|
| **20 detent / 20 pulse per rotasi** | Klik terasa jelas, 18° per klik | ✅ **Paling cocok untuk volume** |
| 24 detent | Sedikit lebih halus | Oke juga |
| High-res (mis. 100+ pulse, optical) | Terlalu banyak event, overkill | ❌ Tidak perlu |

Untuk volume: **20 detent** adalah standar dan feel-nya paling enak.
Firmware menangani repeat-rate, jadi putaran cepat tetap aman.

### 1.3 Shaft (poros)

| Shaft | Panjang | Untuk |
|---|---|---|
| **Knurled (bergerigi)** Ø6 mm | 15–20 mm | ✅ Knob cap press-fit (paling umum, gampang cari knob) |
| D-cut (potong D) Ø6 mm | 15–20 mm | Knob cap dengan set-screw |
| Splined | 15 mm | Knob model lama, agak susah cari tutupnya |

Rekomendasi: **knurled 15–20 mm** + beli knob cap aluminium/plastic yang
cocok (Ø ketik "knob cap EC11" di marketplace, banyak & murah).

### 1.4 Mounting: panel-mount vs PCB-mount

| Tipe | Ciri | Untuk kita |
|---|---|---|
| **Panel-mount (threaded bushing)** | Ada drat M7 + mur + ring di bawah badan | ✅ **Disarankan** — bisa dipasang di bracket/plate apa pun di dashboard |
| PCB-mount (pin lurus ke bawah) | 5 pin vertikal, tanpa mur | Butuh PCB khusus; lebih rapat tapi kurang fleksibel |

Rekomendasi: **panel-mount dengan mur** — encoder dicucuk ke plate/perfboard
kecil di belakang dashboard, dikunci dari depan dengan mur.

### 1.5 Bare vs module (KY-040)

| | Bare EC11 | Module KY-040 |
|---|---|---|
| Isi | Komponen saja, 5 pin | EC11 sudah terpasang di PCB + header pin |
| Resistor pull-up | ❌ Harus tambah sendiri (10kΩ ×3) | ✅ Sudah ada di module |
| Debonce cap | ❌ | ✅ Kadang sudah ada |
| Untuk prototipe | Ribet di breadboard (pin rapat, goyang) | ✅ Enak, tinggal colok dupont |
| Untuk final install | ✅ **Lebih compact, rapi** | Agak besar, PCB module kadang menghalangi |
| Harga | Sangat murah (≈Rp 3–10 rb) | ≈Rp 10–25 rb |

**Keputusan: KY-040** ✅ — dipakai untuk prototipe sekaligus final.
- Pinout: CLK → D5, DT → D6, SW → D7, GND → GND, **VCC → 3V3 (wajib)**, lihat §3.1
- Pull-up sudah ada di module, tidak perlu resistor tambahan
- Bare EC11 panel-mount jadi opsi upgrade nanti kalau mau hasil lebih compact

---

## 2. Daftar komponen (BOM)

| # | Komponen | Spesifikasi | Catatan |
|---|---|---|---|
| 1 | **NodeMCU v3 (ESP8266)** ✅ | ESP8266, USB micro, regulator 5V onboard, pin A0 | A0 = loopback debug ([DEBUG.md](DEBUG.md)) |
| 2 | **KY-040 module** ✅ | EC11 20 detent + push switch, pull-up onboard | Input encoder |
| 3 | **Resistor 4.7 kΩ ×2** | 1/4 watt | Series output KEY1/KEY2 (inline di kabel dupont) |
| 4 | **Kapasitor 100 nF ×2** | keramik (0.1 µF) | Filter RC output KEY1/KEY2 (inline) |
| 5 | Resistor 10 kΩ ×1 (opsional) | 1/4 watt | Dummy pull-up untuk bench simulator ([DEBUG.md](DEBUG.md)) |
| 6 | Knob cap | Ø 6 mm knurled, press-fit | Aluminium/kotak, murah |
| 7 | Kabel | Dupont female-female, kabel ke harness head unit | Output: 4.7k+100nF disisipkan inline, solder ujung + shrink tubing |
| 8 | Perfboard/plate kecil | opsional | Cuma mounting knob ke dashboard, bukan kelistrikan |
| 9 | Multimeter | Untuk kalibrasi & verifikasi | Wajib ada |
| 10 | Power 5V | USB port head unit, atau buck 12V→5V (mini LM2596) | NodeMCU boleh dari 5V pin |

**GPIO yang aman di ESP8266 (NodeMCU):**

| Pin D | GPIO | Status saat boot | Aman untuk |
|---|---|---|---|
| D1 | 5 | float | ✅ Output SWC KEY1 (PWM) |
| D2 | 4 | float | ✅ Output SWC KEY2 (PWM) |
| D5 | 14 | pulldown | ✅ Encoder A |
| D6 | 12 | pulldown | ✅ Encoder B |
| D7 | 13 | pulldown | ✅ Encoder SW (switch) |
| D3 | 0 | **boot strap, harus HIGH saat boot** | ⚠️ hindari |
| D4 | 2 | **boot strap, harus HIGH saat boot** (LED onboard) | ⚠️ hindari |
| D8 | 15 | **boot strap, harus LOW saat boot** | ⚠️ hindari |
| D0 | 16 | tanpa interrupt | ⚠️ hindari untuk encoder |

Jadi: **5 GPIO bersih persis sesuai kebutuhan** — 3 encoder (D5/D6/D7) +
2 output PWM (D1/D2). Catatan: D1/D2 float saat boot → Hi-Z alami sebelum
firmware jalan sama sekali.

---

## 3. Wiring

### 3.1 Encoder → ESP8266

```
KY-040 module                    NodeMCU
───────────                      -------
CLK ────────────────────────────► D5 (GPIO14)   (pin A encoder)
DT  ────────────────────────────► D6 (GPIO12)   (pin B encoder)
SW  ────────────────────────────► D7 (GPIO13)   (push switch)
GND ────────────────────────────► GND
VCC ────────────────────────────► 3V3            (WAJIB — lihat kotak di bawah)
```

> ### ⚠️ VCC (`+`) HARUS ke 3V3 — jangan dibiarkan terbuka
>
> Pull-up ketiga line di modul direferensikan ke pin `+`. Kalau `+` mengambang,
> ketiga line tersambung lewat resistor ke **satu node mengambang**: saat kontak
> encoder menutup (line → GND), node itu ikut tertarik turun → dua line lain
> anjlok ke kisaran ~1–1,7 V, **tepat di ambang input ESP8266** → chattering,
> arah salah, dan SW terbaca "ditekan" tanpa ditekan.
>
> Terbukti dari trace: 22/23 perubahan level SW selalu seiring state CLK/DT
> (lihat DEBUG.md "Temuan 2"). Aturan lama di dokumen ini
> ("VCC jangan dihubungkan") **salah dan sudah dibetulkan** — asumsinya
> "resistor ke node mengambang tidak masalah" keliru.
>
> Pakai **3V3, bukan 5V** — GPIO ESP8266 bukan 5V-toleran.

Catatan:
- Pull-up sudah ada di module — tidak perlu resistor tambahan.
- Kalau suatu saat ganti ke **bare EC11**: label pin A/B/C + SW1/SW2, wajib
  tambah pull-up 10 kΩ ke 3V3 untuk A, B, dan SW (lihat §1.5).

### 3.2 Output SWC → head unit Android (KEY1 / KEY2)

Head unit Android umumnya punya input SWC berupa **resistive ladder**:
pin KEY punya pull-up internal ke Vcc, ADC membaca tegangan — makin rendah
tegangan = makin "kuat" tombol yang terdeteksi. Tiap tombol di SWC learning
diikat ke satu level tegangan/beda resistansi.

Kita tiru dengan **PWM + filter RC** langsung dari GPIO — tanpa modul output:

```
NodeMCU                         (komponen inline di kabel dupont)

D1 (GPIO5) ──[4.7 kΩ]──┬── node KEY1 ──► KEY1 / SWC1 harness
                    100nF──┘       │
                                   └ (pull-up internal head unit, mis. 10k ke 5V)

D2 (GPIO4) ──[4.7 kΩ]──┬── node KEY2 ──► KEY2 / SWC2 harness
                    100nF──┘       │
                                   └ (pull-up internal head unit)

Common: GND NodeMCU ──► GND harness head unit  (WAJIB nyambung)
```

Prinsipnya:
- **Emit (tombol ditekan):** GPIO drive PWM 20 kHz → RC meratakan → tegangan
  DC di node = hasil divider antara resistor series 4.7 kΩ kita dan pull-up
  internal head unit. Duty 0–100% memetakan satu rentang voltase (contoh:
  rail 5V, pull-up 10k → ≈ 1.60V…3.84V) → 3 slot per garis dengan jarak
  ≈ 0.7V. Satu GPIO menghasilkan seluruh ladder, cukup untuk semua slot.
- **Idle (tidak ada tombol):** GPIO di-set mode **INPUT = Hi-Z** → pin
  terputus total → node terangkat penuh ke rail oleh pull-up head unit =
  kondisi "tombol terbuka" yang **persis sempurna**. Rail 3.3V / 5V / 12V
  semuanya otomatis benar — tidak ada phantom, tidak ada fallback.
- 100 nF meratakan ripple PWM (tau ≈ 0.3 ms vs periode 50 µs → ripple < 5%
  rentang) supaya ADC head unit membaca stabil.
- 4.7 kΩ membatasi arus (kalau pull-up 5V, arus ke GPIO 3.3V tetap aman)
  dan menjadi bagian divider yang membentuk level.
- **Level voltase final tidak dihitung dari teori** — hasilnya tergantung
  Rpull head unit yang sebenarnya. Kalibrasi empiris: serial tuner saat
  SWC learning (§4).

### 3.3 Hal yang WAJIB diperiksa sebelum nyambung

1. **Cari pin SWC di harness** — label umum: `KEY1/KEY2`, `SWC1/SWC2`,
   `KEY-A/KEY-B`, `SW1/SW2`. Cek manual/AVL head unit atau pinout ISO.
   Beberapa unit Android memakai konektor mini-USB/aviation untuk SWC.
2. **Ukur tegangan pull-up** di pin KEY — pin KEY yang mengambang (belum
   tersambung apa pun) menunjukkan tegangan rail pull-up-nya, karena tanpa
   arus tidak ada jatuh tegangan di resistor pull-up:

   a. Multimeter mode DC Volt; probe hitam → GND harness.
      Sanity check: ukur dulu ke tegangan yang diketahui (baterai/supply)
      supaya yakin probe benar.
   b. Head unit menyala (ACC ON). Pin KEY1 masih kosong.
   c. Probe merah → KEY1, baca. Ulang KEY2.

   | Bacaan | Arti | Keputusan |
   |---|---|---|
   | ≈ 3.3V | pull-up 3.3V | ✅ lanjut |
   | ≈ 5V   | pull-up 5V   | ✅ lanjut (kasus umum unit Android China) |
   | ≈ 12V  | pull-up 12V  | ❌ STOP, lapor — perlu divider/buffer |
   | ≈ 0V   | tidak terukur | ⚠️ troubleshooting di bawah |

   Opsional — ukur Rpull (head unit mati, mode Ohm, KEY→GND; angka stabil
   ≈ nilai pull-up). Berguna untuk estimasi rentang voltase emit (contoh:
   rail 5V + Rpull 10k → duty 0–1000 ≈ 1.60–3.84V) dan arus loading.
   Nilai final tetap dari kalibrasi empiris, bukan dari hitungan ini.

   Troubleshooting bacaan ≈0V: (1) ulangi sanity check GND probe;
   (2) beberapa unit hanya aktif menyalakan pull-up SWC saat menu SWC
   dibuka — ukur sambil buka menu itu; (3) tetap 0V → kemungkinan skema
   input berbeda (pull-down/aktif-high) — **jangan wiring**, lapor hasilnya.
3. **GND harus common** antara ESP8266 dan harness head unit.
4. Jalur output ke pin KEY harus **lewat resistor 4.7 kΩ + kapasitor 100 nF**
   (§3.2) — jangan pernah hubungkan GPIO langsung ke pin KEY tanpa resistor
   (arus tidak terbatas, bisa bentrok dengan pull-up).

---

## 4. Prosedur SWC learning & kalibrasi

Fitur pendukung: **serial tuner** — firmware membaca perintah serial
`k1 <duty>` / `k2 <duty>` (contoh: `k1 600`, skala 0–1000) untuk mengubah
duty PWM live tanpa reflash. Ini mempercepat proses di bawah.

1. Colok ESP8266 ke PC, buka serial monitor (115200).
2. Buka head unit: **Settings → Steering Wheel Control (SWC) → Start Learning**
   (nama menu bervariasi: "Key Learning", "Steering Wheel Keys").
3. Di layar learning, pilih tombol pertama (mis. VOL+).
4. Dari serial monitor, kirim duty untuk slot-nya (mis. `k1 600`), lalu tekan
   tombol fisik knob sekali → driver emit voltage ~500 ms → layar learning
   menangkap level → tersimpan.
5. Ulangi untuk tiap tombol: geser duty naik/turun sampai tiap tombol
   terdeteksi **stabil dan berbeda** satu sama lain.
6. Catat duty final ke `config.h`, save, reflash sekali di akhir.
7. Verifikasi: tekan tiap gesture → tombol sesuai terdeteksi head unit.

Tips:
- Beri jarak antar slot (≥ 0.3V terukur multimeter/A0) supaya noise aman.
- Kalau 2 tombol terdeteksi sama, naikkan jarak dutynya.
- Emit duration default 500 ms cukup untuk learning; pemakaian normal
  80–120 ms (configurable).
- Idle = Hi-Z → node = rail persis. Verifikasi pertama di Tahap 3: knob tidak
  disentuh, ukur node dengan multimeter → harus membaca rail (bukan bervariasi).

---

## 5. Ringkasan wiring final

```
                 ┌──────────────── NodeMCU v3 ─────────────────────┐
  KY-040         │  D5(GPIO14) ◄── CLK                             │
  ┌─────────┐    │  D6(GPIO12) ◄── DT                             │
  │ EC11    │    │  D7(GPIO13) ◄── SW                             │
  │ +switch │    │  D1(GPIO5) ──[4.7k]──┬── node KEY1 ──► KEY1    │
  └────┬────┘    │                  [100nF]─┤ (ke harness)         │
       │         │  D2(GPIO4) ──[4.7k]──┬── node KEY2 ──► KEY2    │
 CLK,DT,SW,GND ──┤                  [100nF]─┤ (ke harness)         │
  VCC ───────────┼─► 3V3  (WAJIB — node pull-up modul, lihat §3.1) │
                 │  GND ─────────────────────► GND harness         │
                 │  5V ◄── USB head unit / buck 12V→5V            │
                 └─────────────────────────────────────────────────┘

  (4.7k + 100nF per garis disisipkan inline di kabel dupont;
   idle = GPIO mode INPUT / Hi-Z → node tegak di rail pull-up;
   VCC modul ke 3V3 — dibiarkan terbuka = coupling node mengambang)
```
