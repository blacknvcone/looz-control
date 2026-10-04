# USB HID Macro untuk Menjalankan Screensaver

## Tujuan

Memakai pattern tombol pada knob proyek `looz-control` untuk memicu aplikasi
screensaver head unit melalui USB HID. Screensaver dipakai agar display hitam
sementara Android tetap aktif; tujuan alur ini bukan membangunkan Android dari
sleep.

## Fakta dari observasi head unit

- Head unit menjalankan Android 15, API level 35.
- Aplikasi screensaver menghitamkan display, tetapi Android tetap aktif.
- Menyentuh layar menghentikan tampilan screensaver.
- Menggunakan mouse USB juga menghentikan screensaver dan mengembalikan tampilan
  ke launcher.
- Pengguna mengetahui USB HID dapat mengirim kontrol volume naik/turun dan
  next/previous track ke head unit.

Fakta di atas adalah observasi pada unit ini. Belum dilakukan uji RP2040 atau app
mapper.

## Rancangan makro

```text
pattern knob
    → RP2040 membaca pola dan mengirim event USB HID
    → aplikasi mapper Android mengenali event/pola
    → mapper membuka activity aplikasi screensaver
```

USB HID hanya mengirim event input (misalnya pointer, keyboard, atau Consumer
Control). HID tidak mengirim Android `Intent` atau package name secara langsung.
Karena itu, aksi “buka screensaver” harus dikerjakan oleh aplikasi mapper atau
mapping khusus dari firmware/launcher.

## Kemungkinan implementasi tanpa root

Aplikasi Android mapper dapat dibuat untuk mengenali event HID dan meminta
pengguna mengaktifkan `AccessibilityService` melalui Settings. Service tersebut
dapat mengamati key event yang didukung; sesudah cocok dengan pattern, aplikasi
mencoba membuka launcher activity screensaver.

Root belum terlihat sebagai prasyarat intrinsik untuk pendekatan ini. Namun
keberhasilan tanpa root belum terverifikasi pada head unit, terutama untuk:

1. Apakah event Consumer Control yang dikirim RP2040 dapat diamati service secara
   global pada ROM tersebut.
2. Apakah mapper dapat meluncurkan activity screensaver dari background. Android
   membatasi background activity launch; Android 15/API 35 memiliki aturan
   tambahan terkait delegasi izin peluncuran melalui `PendingIntent`.
3. Apakah aplikasi screensaver memiliki launcher activity yang bisa dibuka oleh
   app lain.
4. Apakah port USB tetap aktif sebagai host dan memberi daya saat screensaver
   tampil.

Konfigurasi permission/accessibility pada head unit dapat membantu, tetapi tidak
otomatis menghapus semua batasan background activity launch. Perilaku ROM dan
mekanisme event yang dipakai tetap harus diuji.

## Batasan event HID

- Volume up/down dan play/pause memiliki HID Consumer Control usage standar dan
  pemetaan Android yang terdokumentasi.
- Jangan mengasumsikan semua event media, termasuk next/previous, dapat diterima
  atau ditangkap dengan cara yang sama pada setiap ROM.
- Mouse HID yang mengakhiri screensaver telah diamati secara langsung oleh
  pengguna, tetapi tindakan itu kembali ke launcher; hal itu bukan bukti bahwa
  mouse HID dapat membuka app screensaver.
- Event trigger untuk mapper sebaiknya dipilih dan diverifikasi agar tidak
  mengganggu fungsi knob lain (volume/media).

## Perangkat keras proyek

Firmware yang ada saat ini menggunakan NodeMCU v3 / ESP8266. Board tersebut tidak
menyediakan USB device HID native melalui port micro-USB yang menggunakan
USB-to-serial bridge. Untuk HID langsung kemungkinan dibutuhkan board dengan USB
device controller yang mendukung HID, misalnya RP2040. Belum ada perubahan
firmware atau hardware dalam tahap riset ini.

## Riset/uji yang masih diperlukan

1. Identifikasi package name dan exported launcher activity aplikasi screensaver.
2. Verifikasi event USB HID yang ingin dijadikan trigger dan apakah app mapper
   dapat menerimanya saat launcher/screensaver berada di foreground.
3. Verifikasi AccessibilityService dapat diaktifkan pada ROM head unit.
4. Uji apakah mapper dapat membuka screensaver activity pada Android 15/API 35
   saat berjalan di background; periksa logcat bila peluncuran diblokir.
5. Pastikan USB host tetap aktif/berdaya dalam kondisi layar screensaver.
6. Baru setelah itu putuskan skema pattern knob dan hardware USB HID.

## Sumber platform

- AOSP, USB HID mappings untuk USB headsets: <https://source.android.com/docs/core/interaction/accessories/headset/usb-device>
- AOSP, Android input pipeline: <https://source.android.com/docs/core/interaction/input>
- Android `AccessibilityService`: <https://developer.android.com/reference/android/accessibilityservice/AccessibilityService>
- Android 15 background activity launch changes: <https://developer.android.com/about/versions/15/behavior-changes-15>
- Android background activity launch restrictions: <https://developer.android.com/guide/components/activities/secure-bal>

## Status

Riset/dokumentasi saja. Belum ada perubahan kode, firmware, konfigurasi head unit,
atau pengujian perangkat untuk alur macro ini.
