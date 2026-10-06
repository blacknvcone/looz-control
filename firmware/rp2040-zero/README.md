# RP2040 Zero USB HID firmware

Active firmware target: Waveshare RP2040 Zero with the Arduino-Pico core and its **built-in Pico SDK USB stack**. In the Arduino IDE select the RP2040 Zero-compatible board entry, install/select Arduino-Pico, and ensure the core uses built-in USB (not the Adafruit TinyUSB stack) for `Keyboard.h`.

## Build status

The sketch has been compile-verified with Arduino CLI 1.5.1 and Arduino-Pico core 6.2.0 using FQBN `rp2040:rp2040:waveshare_rp2040_zero`. The HID calls use the built-in Pico SDK USB stack. Firmware has not yet been flashed or tested on hardware.

Compile from this directory:

```sh
arduino-cli compile --fqbn rp2040:rp2040:waveshare_rp2040_zero .
```

To flash, put the board into BOOTSEL mode (hold BOOT while connecting USB-C, then release), identify it with `arduino-cli board list`, and upload using the UF2 bootloader/port selected by Arduino CLI. Do not assume a serial port is present before the first upload. Physical upload is pending board detection.

## USB identity and HID behavior

The device enumerates as a USB HID keyboard/media-key device through the selected core's defaults. With this board/core build, the compile command reports the board defaults VID `0x2e8a`, PID `0x0003`, manufacturer `Waveshare`, product `RP2040 Zero`; the sketch does **not** assign them. Do not claim a project-owned identity or set an arbitrary VID/PID. Before distributing hardware, use a properly assigned VID/PID and check the core's USB identity configuration.

The onboard WS2812 RGB LED on **GP16** gives a short green blink (80 ms) whenever a HID input action is emitted. Its brightness is limited in firmware for a small status indication.

HID sends input reports, not Android intents, app package names, or launch requests. Opening a screensaver remains the responsibility of an Android mapper/launcher. The firmware maps single/double/triple encoder presses to play/pause, next, and previous media keys; rotation maps to volume up/down. A long-press (500 ms) sends Consumer Control usage `0x0223` (AC Home) as a pattern trigger, then sends release. The intended host-side setup is to remap that event with the keyboard remapping app installed on the head unit. Validate that the app can intercept this Consumer Control usage and map it to the desired action.

## Pin mapping

| RP2040 Zero | KY-040 | Notes |
|---|---|---|
| GP2 | CLK | Encoder channel A |
| GP3 | DT | Encoder channel B |
| GP4 | SW | Push switch, active low |
| 3V3 | VCC / `+` | Never power the module from 5 V |
| GND | GND | Common ground |

See [../../docs/wiring-rp2040-zero-ky040.md](../../docs/wiring-rp2040-zero-ky040.md) and [../../docs/wiring-rp2040-zero-ky040.svg](../../docs/wiring-rp2040-zero-ky040.svg).
