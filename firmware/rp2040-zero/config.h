#pragma once

// RP2040 Zero / Arduino-Pico core GPIO assignment. All KY-040 signals stay at 3.3 V.
#define PIN_ENC_CLK  2  // GP2, KY-040 CLK
#define PIN_ENC_DT   3  // GP3, KY-040 DT
#define PIN_ENC_SW   4  // GP4, KY-040 push switch (active low)

#define BUTTON_DEBOUNCE_MS  30
#define CLICK_WINDOW_MS    400
#define LONG_PRESS_MS      500
#define VOLUME_REPEAT_MS   120
#define MAX_VOLUME_QUEUE    10
#define PIN_STATUS_LED      16  // onboard WS2812 RGB LED data pin
#define STATUS_LED_PULSE_MS 80

// Long-press is sent as a Consumer Control usage only after 500 ms.
// 0x0223 = AC Home. This is a standard HID usage; configure the Android
// host mapper to recognize it and open the desired screensaver action.
#define LONG_PRESS_HID_USAGE 0x0223
