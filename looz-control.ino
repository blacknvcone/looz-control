// looz-control — control knob SWC untuk head unit Android
// Tahap 1: input (encoder + tombol) + driver output PWM/Hi-Z + serial console
// Detail: PLAN.md / HARDWARE.md / DEBUG.md
#include <Arduino.h>
#include "config.h"

// ============================================================
// LOG
// ============================================================
static volatile int logLevel = LOG_DEFAULT;
#define LOG(lvl, ...)                                          \
  do {                                                         \
    if (logLevel >= (lvl)) {                                   \
      Serial.printf("[%7lu] ", (unsigned long)millis());       \
      Serial.printf(__VA_ARGS__);                              \
      Serial.println();                                        \
    }                                                          \
  } while (0)

// ============================================================
// ENCODER — quadrature state machine (interrupt CHANGE di CLK & DT)
// Decoder lama (edge naik CLK saja + debounce 1500 µs) TERBUKTI salah arah:
// bounce kontak CLK saat putar kiri terbaca CW → VOL+ sesekali.
// (bukti: DEBUG.md "Temuan bug arah" + traces/)
// Prinsip: transisi sah = tepat SATU bit berubah; bounce menghasilkan
// transisi + balikannya → saling cancel; transisi 2-bit = tidak sah.
// ============================================================
static volatile int32_t encDelta = 0;      // detent: +1 = CW, -1 = CCW
static volatile uint8_t  encPrev = 0;      // state sebelumnya (CLK<<1 | DT)
static volatile int8_t   encQuarter = 0;   // akumulasi 1/4 detent (±4 = 1 detent)
static volatile uint32_t encLastUs = 0;
static int32_t encDetentTotal = 0;         // total detent utk log (boleh minus)

// Trace mentah (hanya log level 3): ISR menulis, loop yang mencetak.
// Dipakai memverifikasi arah — lihat DEBUG.md "Trace encoder".
struct EncTrace {
  uint32_t us;        // micros() saat ISR berjalan
  int32_t  gap;       // µs sejak edge terakhir yang DIPROSES
  uint8_t  clk;       // level CLK saat itu (0/1)
  uint8_t  dt;        // level DT saat itu (0/1)
  int8_t   dir;       // arah transisi ini (+1 CW / -1 CCW / 0 = tidak sah)
  uint8_t  verdict;   // 0 = transisi tidak sah, 1 = tolak debounce, 2 = diterima
};
static EncTrace traceBuf[ENC_TRACE_LEN];
static volatile uint16_t traceHead = 0;
static volatile uint16_t traceTail = 0;
static volatile uint16_t traceDrop = 0;

void IRAM_ATTR tracePush(const EncTrace &e) {
  uint16_t next = (uint16_t)((traceHead + 1) % ENC_TRACE_LEN);
  if (next == traceTail) { traceDrop++; return; }
  traceBuf[traceHead] = e;
  traceHead = next;
}

void IRAM_ATTR isrEncoder() {
  uint32_t now = micros();
  int32_t gap = (int32_t)(now - encLastUs);
  uint8_t clk = digitalRead(PIN_ENC_CLK) == HIGH;
  uint8_t dt  = digitalRead(PIN_ENC_DT)  == HIGH;
  uint8_t curr = (uint8_t)((clk << 1) | dt);
  uint8_t prev = encPrev;
  uint8_t diff = (uint8_t)(prev ^ curr);

  // arah transisi ini (dihitung juga utk trace; dipakai hanya bila sah)
  int8_t dir = 0;
  if (diff && diff != 0b11) {                    // tepat SATU bit berubah
    uint8_t x = (uint8_t)(clk ^ dt);
    dir = (diff == 0b10) ? (x ? +1 : -1)         // CLK berubah
                         : (x ? -1 : +1);        // DT berubah
    if (ENC_INVERT) dir = -dir;
  }

  // State SELALU di-update (prev tidak boleh basi: dua transisi nyata di dalam
  // jendela akan tampak lompat 2-bit kalau state tertinggal → detent hilang).
  // Yang diberi jendela hanya PENGHITUNGANNYA.
  encPrev = curr;

  uint8_t verdict;
  if (!dir)                    verdict = 0;     // diff 0 / 2-bit (ISR telat)
  else if (gap < ENC_DEBOUNCE_US) verdict = 1;  // bounce di dalam jendela
  else {
    verdict = 2;
    encLastUs = now;                            // jendela diukur dari edge terhitung
    encQuarter = (int8_t)(encQuarter + dir);
    while (encQuarter >= 4)  { encDelta += 1; encQuarter = (int8_t)(encQuarter - 4); }
    while (encQuarter <= -4) { encDelta -= 1; encQuarter = (int8_t)(encQuarter + 4); }
  }
  if (logLevel >= 3) tracePush({ now, gap, clk, dt, dir, verdict });
}

static void traceDrain() {
  for (;;) {
    EncTrace e;
    noInterrupts();
    if (traceTail == traceHead) { interrupts(); break; }
    e = traceBuf[traceTail];
    traceTail = (uint16_t)((traceTail + 1) % ENC_TRACE_LEN);
    interrupts();
    static const char *V[3] = { "transisi tidak sah", "tolak debounce", "diterima" };
    LOG(3, "TRC   t=%luus gap=%ld dir=%s CLK=%d DT=%d -> %s",
        (unsigned long)e.us, (long)e.gap,
        e.dir > 0 ? "CW " : (e.dir < 0 ? "CCW" : "--"), e.clk, e.dt, V[e.verdict]);
  }
  noInterrupts();
  uint16_t d = traceDrop;
  traceDrop = 0;
  interrupts();
  if (d) LOG(3, "TRC   %u edge hilang (buffer penuh)", (unsigned)d);
}

// ============================================================
// LED INDIKATOR — LED onboard NodeMCU (D4, aktif rendah)
// Jumlah blink = nomor slot + 1 (1=VOL- … 7=SLEEP), non-blocking
// ============================================================
static uint8_t ledRemain = 0;         // sisa blink
static bool ledPhaseOn = false;       // sedang fase nyala?
static uint32_t ledPhaseUntil = 0;    // kapan fase berikutnya

static void ledNotify(int slot) {
  ledRemain = (uint8_t)(slot + 1);
  ledPhaseOn = true;
  ledPhaseUntil = millis() + LED_ON_MS;
  digitalWrite(PIN_LED, LOW);         // aktif rendah = nyala
}

static void ledTick() {
  if (!ledRemain) return;
  uint32_t now = millis();
  if ((int32_t)(now - ledPhaseUntil) < 0) return;
  if (ledPhaseOn) {
    ledRemain--;                      // satu blink selesai
    digitalWrite(PIN_LED, HIGH);      // mati
    ledPhaseOn = false;
    if (!ledRemain) return;
    ledPhaseUntil = now + LED_OFF_MS;
  } else {
    ledPhaseOn = true;
    digitalWrite(PIN_LED, LOW);
    ledPhaseUntil = now + LED_ON_MS;
  }
}

// ============================================================
// ENGINE OUTPUT — per garis: mode hold (tuner) / pulse (gesture) / Hi-Z
// ============================================================
struct LineState {
  bool held;        // hold tuner aktif (k1/k2) — menang atas pulse
  int holdDuty;
  bool pulse;
  uint32_t until;
};
static LineState lines[3] = {};   // index 1 (KEY1) dan 2 (KEY2)

static int pinOf(int ln) { return ln == 1 ? PIN_KEY1 : PIN_KEY2; }

static void lineHiZ(int ln) {
  pinMode(pinOf(ln), INPUT);      // dilepas total = kondisi "tanpa tombol"
}

static void emitSlot(int slot, uint32_t ms) {
  Slot &s = slots[slot];
  int ln = s.line;
  if (lines[ln].held) {
    LOG(3, "ACT   slot=%s ditahan (tuner k%d aktif) — diabaikan", s.name, ln);
    return;
  }
  pinMode(pinOf(ln), OUTPUT);
  analogWrite(pinOf(ln), s.duty);
  lines[ln].pulse = true;
  lines[ln].until = millis() + ms;
  ledNotify(slot);
  // Ukur node via loopback A0 — bermakna hanya bila node di-jumper ke A0.
  // raw < 30 ≈ tidak ada jumper (A0 floating), laporkan sebagai "--".
  delay(5);                               // tunggu RC settle (tau ~0.47ms)
  int raw = analogRead(PIN_LOOPBACK);
  char meas[16];
  if (raw >= 30) {
    int mv = (int)(raw * ADC_SCALE * 1000.0f + 0.5f);
    snprintf(meas, sizeof(meas), "%d.%03dV", mv / 1000, mv % 1000);
  } else {
    snprintf(meas, sizeof(meas), "--");
  }
  LOG(1, "ACT   slot=%s line=KEY%d duty=%d meas=%s emit %lums",
      s.name, ln, s.duty, meas, (unsigned long)ms);
}

static void holdLine(int ln, int duty) {
  duty = constrain(duty, 0, PWM_RANGE);
  lines[ln].held = true;
  lines[ln].holdDuty = duty;
  lines[ln].pulse = false;
  pinMode(pinOf(ln), OUTPUT);
  analogWrite(pinOf(ln), duty);
  LOG(1, "OK    KEY%d hold duty=%d", ln, duty);
}

static void releaseLine(int ln) {
  lines[ln].held = false;
  lines[ln].pulse = false;
  lineHiZ(ln);
  LOG(1, "IDLE  KEY%d Hi-Z", ln);
}

static void engineTick() {
  uint32_t now = millis();
  for (int ln = 1; ln <= 2; ln++) {
    if (lines[ln].pulse && (int32_t)(now - lines[ln].until) >= 0) {
      lines[ln].pulse = false;
      if (!lines[ln].held) {
        lineHiZ(ln);
        LOG(2, "REL   KEY%d release → Hi-Z", ln);
      }
    }
  }
}

// ============================================================
// PETA GESTURE → SLOT + resolusi klik
// ============================================================
static void emitGesture(int act, uint32_t ms) { emitSlot(act, ms); }

static void resolveClicks(int n) {
  const char *how = "";
  int act = -1;
  switch (n) {
    case 1: act = ACT_1PRESS;    how = "1 press";  break;
    case 2: act = ACT_2PRESS;    how = "2 press";  break;
    case 3: act = ACT_3PRESS;    how = "3 press";  break;
    default:
      LOG(2, "BTN   klik x%d diabaikan (maks 3)", n);
      return;
  }
  LOG(2, "BTN   resolve clicks=%d (%s) -> %s", n, how, slots[act].name);
  emitGesture(act, EMIT_DEFAULT_MS);
}

// ============================================================
// TOMBOL TEKAN — FSM: klik saat lepas, long-press saat tahan
// ============================================================
enum BtnPhase { BTN_WAIT, BTN_PRESSED, BTN_WIN };
static BtnPhase btnPhase = BTN_WAIT;
static bool btnLastRaw = false;
static uint32_t btnLastChange = 0;
static uint32_t btnDownAt = 0;
static bool btnLongFired = false;
static int clickCount = 0;
static uint32_t clickDeadline = 0;

static void btnPoll() {
  bool raw = (digitalRead(PIN_ENC_SW) == LOW);   // aktif rendah
  uint32_t now = millis();

  if (raw != btnLastRaw) {
    btnLastRaw = raw;
    btnLastChange = now;
    // telusur false-press: level SW berubah saat state encoder apa?
    LOG(3, "SWC   raw=%d CLK=%d DT=%d", raw,
        digitalRead(PIN_ENC_CLK) == HIGH, digitalRead(PIN_ENC_DT) == HIGH);
  }
  if (now - btnLastChange < BTN_DEBOUNCE_MS) return;
  bool stable = raw;

  switch (btnPhase) {
    case BTN_WAIT:
      if (stable) {
        btnPhase = BTN_PRESSED;
        btnDownAt = now;
        btnLongFired = false;
        LOG(2, "BTN   press");
      }
      break;

    case BTN_PRESSED:
      if (!stable) {                          // dilepas
        if (btnLongFired) {
          btnPhase = BTN_WAIT;
          LOG(2, "BTN   release (setelah longpress)");
        } else {
          clickCount++;
          btnPhase = BTN_WIN;
          clickDeadline = now + BTN_CLICK_WINDOW_MS;
          LOG(2, "BTN   release clicks=%d", clickCount);
        }
      } else if (!btnLongFired && now - btnDownAt >= BTN_LONGPRESS_MS) {
        btnLongFired = true;
        LOG(2, "BTN   longpress >=%dms -> %s",
            BTN_LONGPRESS_MS, slots[ACT_LONGPRESS].name);
        emitGesture(ACT_LONGPRESS, EMIT_DEFAULT_MS);
      }
      break;

    case BTN_WIN:
      if (stable) {                           // tekan lagi sebelum jendela tutup
        btnPhase = BTN_PRESSED;
        btnDownAt = now;
        btnLongFired = false;
        LOG(2, "BTN   press");
      } else if ((int32_t)(now - clickDeadline) >= 0) {
        int n = clickCount;
        clickCount = 0;
        btnPhase = BTN_WAIT;
        resolveClicks(n);
      }
      break;
  }
}

// ============================================================
// VOLUME — antrean pulse ber-repeat-rate selama diputar
// ============================================================
static int volAccum = 0;
static uint32_t lastVolEmit = 0;

static void volumeTick() {
  if (volAccum == 0) return;
  uint32_t now = millis();
  if (now - lastVolEmit < VOL_REPEAT_MS) return;
  if (volAccum > 0) { emitGesture(ACT_ROT_RIGHT, EMIT_DEFAULT_MS); volAccum--; }
  else              { emitGesture(ACT_ROT_LEFT,  EMIT_DEFAULT_MS); volAccum++; }
  lastVolEmit = now;
}

static void encoderTick() {
  int32_t d;
  noInterrupts();
  d = encDelta;
  encDelta = 0;
  interrupts();
  if (d == 0) return;

  encDetentTotal += d;
  LOG(2, "ENC   dir=%s detent=%ld",
      d > 0 ? "CW " : "CCW", (long)encDetentTotal);

  volAccum += d;
  if (volAccum > VOL_QUEUE_MAX)  volAccum = VOL_QUEUE_MAX;
  if (volAccum < -VOL_QUEUE_MAX) volAccum = -VOL_QUEUE_MAX;
}

// ============================================================
// SERIAL CONSOLE
// ============================================================
static bool ieq(const char *a, const char *b) {   // case-insensitive equals
  while (*a && *b) {
    char ca = (*a >= 'A' && *a <= 'Z') ? *a + 32 : *a;
    char cb = (*b >= 'A' && *b <= 'Z') ? *b + 32 : *b;
    if (ca != cb) return false;
    a++; b++;
  }
  return *a == 0 && *b == 0;
}

static int slotByName(const char *n) {
  for (int i = 0; i < SLOT_COUNT; i++)
    if (ieq(slots[i].name, n)) return i;
  return -1;
}

static void cmdHelp() {
  Serial.println(F(
    "perintah serial:\n"
    "  help              daftar perintah\n"
    "  log 0..3          verbosity (0=mati,1=action,2=+input,3=+debug)\n"
    "  adc               baca voltase loopback A0\n"
    "  k1 <duty>         hold KEY1 di duty 0..1000 (learning)\n"
    "  k2 <duty>         hold KEY2 di duty 0..1000 (learning)\n"
    "  emit <slot> [ms]  pulse slot sekali (default 500ms)\n"
    "  test <gesture>    injeksi: vol- vol+ 1 dbl tlb long\n"
    "  map               tabel gesture -> slot\n"
    "  idle              lepas semua output (Hi-Z)"));
}

static void cmdMap() {
  Serial.println(F("gesture              slot    line  duty"));
  const struct { const char *g; int act; } m[] = {
    {"putar kiri ", ACT_ROT_LEFT}, {"putar kanan", ACT_ROT_RIGHT},
    {"1 tekan    ", ACT_1PRESS},   {"2 tekan    ", ACT_2PRESS},
    {"3 tekan    ", ACT_3PRESS},   {"long press ", ACT_LONGPRESS},
  };
  for (auto &e : m)
    Serial.printf("  %-12s -> %-6s KEY%d  %d\n",
                  e.g, slots[e.act].name, slots[e.act].line, slots[e.act].duty);
}

static void cmdAdc() {
  int raw = analogRead(PIN_LOOPBACK);
  int mv = (int)(raw * ADC_SCALE * 1000.0f + 0.5f);
  Serial.printf("ADC   raw=%d -> %d.%03dV  (node yang di-jumper ke A0)\n",
                raw, mv / 1000, mv % 1000);
}

static void cmdEmit(char *arg1, char *arg2) {
  if (!arg1) { Serial.println(F("usage: emit <slot> [ms]")); return; }
  int s = slotByName(arg1);
  if (s < 0) { Serial.println(F("slot tidak dikenal — lihat 'map'")); return; }
  uint32_t ms = arg2 ? (uint32_t)atoi(arg2) : (uint32_t)EMIT_LEARN_MS;
  if (ms < 20) ms = 20;
  emitSlot(s, ms);
}

static void cmdTest(char *arg) {
  if (!arg) { Serial.println(F("usage: test vol- vol+ 1 dbl tlb long")); return; }
  LOG(1, "TEST  inject gesture=%s", arg);
  if      (ieq(arg, "vol-")) emitGesture(ACT_ROT_LEFT, EMIT_DEFAULT_MS);
  else if (ieq(arg, "vol+")) emitGesture(ACT_ROT_RIGHT, EMIT_DEFAULT_MS);
  else if (ieq(arg, "1"))    resolveClicks(1);
  else if (ieq(arg, "dbl"))  resolveClicks(2);
  else if (ieq(arg, "tlb"))  resolveClicks(3);
  else if (ieq(arg, "long")) {
    LOG(2, "BTN   longpress (injected) -> %s", slots[ACT_LONGPRESS].name);
    emitGesture(ACT_LONGPRESS, EMIT_DEFAULT_MS);
  }
  else Serial.println(F("gesture tidak dikenal: vol- vol+ 1 dbl tlb long"));
}

static void runCmd(char *line) {
  char *cmd = strtok(line, " ");
  char *a1  = strtok(nullptr, " ");
  char *a2  = strtok(nullptr, " ");

  if (ieq(cmd, "help"))       cmdHelp();
  else if (ieq(cmd, "log")) {
    if (!a1) { Serial.println(F("usage: log 0..3")); return; }
    logLevel = constrain(atoi(a1), 0, 3);
    Serial.printf("OK    log level=%d\n", logLevel);
  }
  else if (ieq(cmd, "adc"))   cmdAdc();
  else if (ieq(cmd, "k1") || ieq(cmd, "k2")) {
    if (!a1) { Serial.printf("usage: %s <duty 0..%d>\n", cmd, PWM_RANGE); return; }
    holdLine(ieq(cmd, "k1") ? 1 : 2, atoi(a1));
  }
  else if (ieq(cmd, "emit"))  cmdEmit(a1, a2);
  else if (ieq(cmd, "test"))  cmdTest(a1);
  else if (ieq(cmd, "map"))   cmdMap();
  else if (ieq(cmd, "idle")) {
    releaseLine(1);
    releaseLine(2);
    Serial.println(F("IDLE  KEY1=Hi-Z KEY2=Hi-Z"));
  }
  else Serial.println(F("unknown command — ketik 'help'"));
}

static char serBuf[40];
static uint8_t serLen = 0;

static void serialTick() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (serLen) {
        serBuf[serLen] = 0;
        runCmd(serBuf);
        serLen = 0;
      }
    } else if (serLen < sizeof(serBuf) - 1) {
      serBuf[serLen++] = c;
    }
  }
}

// ============================================================
// SETUP / LOOP
// ============================================================
void setup() {
  // Output Hi-Z SEJAK DETIK PERTAMA — D1/D2 juga float alami saat boot
  lineHiZ(1);
  lineHiZ(2);

  // LED indikator: matikan dulu BARU jadi output (D4 = boot-strap, aktif rendah)
  digitalWrite(PIN_LED, HIGH);
  pinMode(PIN_LED, OUTPUT);

  pinMode(PIN_ENC_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC_DT,  INPUT_PULLUP);
  pinMode(PIN_ENC_SW,  INPUT_PULLUP);

  analogWriteFreq(PWM_FREQ_HZ);
  analogWriteRange(PWM_RANGE);

  Serial.begin(SERIAL_BAUD);
  delay(80);
  Serial.println();
  Serial.println(F("=== looz-control v0.1 (tahap 1: input + console) ==="));
  Serial.printf("PWM %d Hz, range 0..%d, emit %dms, longpress %dms, window %dms\n",
                PWM_FREQ_HZ, PWM_RANGE, EMIT_DEFAULT_MS,
                BTN_LONGPRESS_MS, BTN_CLICK_WINDOW_MS);
  cmdMap();
  Serial.println(F("ketik 'help' untuk perintah serial"));
  // State machine quadrature: init state dulu, lalu CHANGE di kedua channel
  uint8_t clk0 = digitalRead(PIN_ENC_CLK) == HIGH;
  uint8_t dt0  = digitalRead(PIN_ENC_DT)  == HIGH;
  encPrev = (uint8_t)((clk0 << 1) | dt0);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_CLK), isrEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_DT),  isrEncoder, CHANGE);
}

void loop() {
  encoderTick();
  traceDrain();
  btnPoll();
  volumeTick();
  ledTick();
  engineTick();
  serialTick();
}
