// looz-control — konfigurasi (SEMUA yang bisa diubah ada di file ini)
// Target: NodeMCU v3 (ESP8266) + KY-040 → SWC KEY1/KEY2 head unit Android
#pragma once
#include <Arduino.h>

// ============================================================
// PIN (NodeMCU v3) — jangan dipindah tanpa cek HARDWARE.md §3
// ============================================================
#define PIN_ENC_CLK   D5   // GPIO14 — KY-040 CLK (A)
#define PIN_ENC_DT    D6   // GPIO12 — KY-040 DT (B)
#define PIN_ENC_SW    D7   // GPIO13 — KY-040 push switch
#define PIN_KEY1      D1   // GPIO5  — output PWM → 4.7k → 100nF → KEY1
#define PIN_KEY2      D2   // GPIO4  — output PWM → 4.7k → 100nF → KEY2
#define PIN_LOOPBACK  A0   //        — jumper bench saja (lihat DEBUG.md)
#define PIN_LED       D4   // LED onboard NodeMCU (GPIO2) — aktif rendah
                           // boot-strap: tulis HIGH dulu sebelum jadi OUTPUT

#define ENC_INVERT    0    // 1 kalau arah putaran kebalik (CW ↔ CCW)

// ============================================================
// TIMING
// ============================================================
#define ENC_DEBOUNCE_US      1500   // jendela antar edge TERHITUNG (µs); bounce
                                    // di dalamnya tidak dihitung, tapi state
                                    // tetap di-update (prev tidak boleh basi)
#define ENC_TRACE_LEN         256   // antrian trace mentah edge (log level 3)
#define LED_ON_MS              70   // fase nyala blink LED per gesture
#define LED_OFF_MS             70   // fase mati antar blink
#define BTN_DEBOUNCE_MS        30   // debounce tombol tekan
#define BTN_CLICK_WINDOW_MS   400   // jendela tunggu multi-click
#define BTN_LONGPRESS_MS      500   // tahan ≥ ini = long-press
#define EMIT_DEFAULT_MS       100   // durasi pulse gesture normal
#define EMIT_LEARN_MS         500   // durasi default perintah 'emit' (learning)
#define VOL_REPEAT_MS         120   // repeat-rate volume selama diputar
#define VOL_QUEUE_MAX          10   // antrean pulse volume (maks berputar)

// ============================================================
// OUTPUT — PWM 20 kHz + RC (4.7k / 100nF)
// ============================================================
#define PWM_FREQ_HZ   20000
#define PWM_RANGE     1000          // duty 0..1000 (0.1% per step)
#define ADC_SCALE     0.00547f      // voltase = raw × scale; kalibrasi via
                                    // perintah 'adc' (lihat DEBUG.md)
#define DUTY_MIN_GAP  150           // jarak minimal antar slot (rekomendasi)

// ============================================================
// SLOT TOMBOL — garis + duty per slot (hasil kalibrasi SWC learning)
// line: 1 = KEY1, 2 = KEY2
// ============================================================
enum SlotId {
  SLOT_VOL_DOWN, SLOT_VOL_UP, SLOT_PLAY, SLOT_MUTE,
  SLOT_NEXT, SLOT_PREV, SLOT_SLEEP, SLOT_COUNT
};

struct Slot {
  const char *name;
  uint8_t line;   // 1 = KEY1, 2 = KEY2
  int duty;       // 0..PWM_RANGE
};

inline Slot slots[SLOT_COUNT] = {
  // name      line  duty   (duty awal = placeholder — ganti hasil learning)
  {"VOL-",      1,   300 },
  {"VOL+",      1,   550 },
  {"PLAY",      1,   800 },
  {"MUTE",      1,   800 },   // jarang dipakai — duty sama dgn PLAY, jarak
                              // diperiksa via 'map' saat learning
  {"NEXT",      2,   300 },
  {"PREV",      2,   550 },
  {"SLEEP",     2,   800 },
};

// ============================================================
// PEMETAAN GESTURE → SLOT
// ============================================================
#define ACT_ROT_LEFT   SLOT_VOL_DOWN   // putar kiri
#define ACT_ROT_RIGHT  SLOT_VOL_UP     // putar kanan
#define ACT_1PRESS     SLOT_PLAY       // 1 tekan (ganti SLOT_MUTE utk mute)
#define ACT_2PRESS     SLOT_NEXT       // 2 tekan
#define ACT_3PRESS     SLOT_PREV       // 3 tekan
#define ACT_LONGPRESS  SLOT_SLEEP      // long-press ≥ 500ms

// ============================================================
// SERIAL
// ============================================================
#define SERIAL_BAUD   115200
// log level: 0=mati, 1=action saja, 2=+input, 3=+debug
#define LOG_DEFAULT   2
