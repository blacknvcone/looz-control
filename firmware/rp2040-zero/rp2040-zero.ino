// looz-control — RP2040 Zero USB HID controller scaffold
// Arduino-Pico core; select the built-in Pico SDK USB stack (not Adafruit TinyUSB).
#include <Arduino.h>
#include <Keyboard.h>
#include "config.h"

static uint8_t previousAB = 0;
static int8_t quarterSteps = 0;
static uint32_t lastButtonChange = 0;
static bool lastButton = false;
static uint32_t pressedAt = 0;
static uint32_t clickDeadline = 0;
static uint8_t pendingClicks = 0;
static bool longSent = false;
static bool stablePressed = false;
static uint32_t lastVolumeAt = 0;
static int8_t volumeQueue = 0;

static void sendConsumerUsage(uint16_t usage) {
  Keyboard.consumerPress(usage);
  delay(8);
  Keyboard.consumerRelease();
}
// Full quadrature transition table: invalid two-bit transitions contribute 0.
// This avoids interpreting noise as a detent and correctly accumulates reversals.
static const int8_t QUADRATURE[16] = {
   0, -1, +1,  0,
  +1,  0,  0, -1,
  -1,  0,  0, +1,
   0, +1, -1,  0,
};

static void onClickCount(uint8_t count) {
  // Arduino-Pico Keyboard.h provides the documented media-key usages.
  switch (count) {
    case 1: sendConsumerUsage(KEY_PLAY_PAUSE); break;
    case 2: sendConsumerUsage(KEY_SCAN_NEXT); break;
    case 3: sendConsumerUsage(KEY_SCAN_PREVIOUS); break;
    default: break;
  }
}

static void pollEncoder() {
  uint8_t ab = ((digitalRead(PIN_ENC_CLK) == HIGH) << 1) |
               (digitalRead(PIN_ENC_DT) == HIGH);
  quarterSteps += QUADRATURE[(previousAB << 2) | ab];
  while (quarterSteps >= 4) {
    if (volumeQueue < MAX_VOLUME_QUEUE) volumeQueue++;
    quarterSteps -= 4;
  }
  while (quarterSteps <= -4) {
    if (volumeQueue > -MAX_VOLUME_QUEUE) volumeQueue--;
    quarterSteps += 4;
  }
  previousAB = ab;

  uint32_t now = millis();
  if (volumeQueue && now - lastVolumeAt >= VOLUME_REPEAT_MS) {
    if (volumeQueue > 0) {
      sendConsumerUsage(KEY_VOLUME_INCREMENT);
      volumeQueue--;
    } else {
      sendConsumerUsage(KEY_VOLUME_DECREMENT);
      volumeQueue++;
    }
    lastVolumeAt = now;
  }
}

static void pollButton() {
  const uint32_t now = millis();
  const bool raw = digitalRead(PIN_ENC_SW) == LOW;
  if (raw != lastButton) {
    lastButton = raw;
    lastButtonChange = now;
  }
  if (now - lastButtonChange < BUTTON_DEBOUNCE_MS) return;

  if (raw != stablePressed && now - lastButtonChange >= BUTTON_DEBOUNCE_MS) {
    stablePressed = raw;
    if (stablePressed) {
      pressedAt = now;
      longSent = false;
    } else if (!longSent) {
      pendingClicks++;
      clickDeadline = now + CLICK_WINDOW_MS;
    }
  }
  if (stablePressed && !longSent && now - pressedAt >= LONG_PRESS_MS) {
    longSent = true;
    // Long-press action intentionally remains a host-mapper decision.
  }
  if (!stablePressed && pendingClicks && (int32_t)(now - clickDeadline) >= 0) {
    onClickCount(pendingClicks);
    pendingClicks = 0;
  }
}

void setup() {
  pinMode(PIN_ENC_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC_DT, INPUT_PULLUP);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);
  lastButton = digitalRead(PIN_ENC_SW) == LOW;
  stablePressed = lastButton;
  lastButtonChange = millis();
  previousAB = ((digitalRead(PIN_ENC_CLK) == HIGH) << 1) |
               (digitalRead(PIN_ENC_DT) == HIGH);
  Keyboard.begin();
}

void loop() {
  pollEncoder();
  pollButton();
}
