# looz-control

Rotary-knob USB HID controller for an Android head unit. **Active target: Waveshare RP2040 Zero.** It reads a KY-040 encoder and emits USB keyboard/media-key reports through the board's native USB device interface.

The previous NodeMCU/ESP8266 steering-wheel-control (SWC) voltage-ladder project is preserved as legacy in [`firmware/esp8266-swc/`](firmware/esp8266-swc/README.md); its source, notes, diagrams, tools, and trace logs are retained. It is not the active firmware target.

## Active RP2040 Zero project

- Firmware scaffold: [`firmware/rp2040-zero/`](firmware/rp2040-zero/README.md)
- Wiring guide: [`docs/wiring-rp2040-zero-ky040.md`](docs/wiring-rp2040-zero-ky040.md)
- Diagram: [`docs/wiring-rp2040-zero-ky040.svg`](docs/wiring-rp2040-zero-ky040.svg)
- Pin map: GP2=CLK, GP3=DT, GP4=SW; KY-040 VCC to 3V3 and GND to GND.
- USB reports are HID input events only. HID does not launch Android apps or send intents; screensaver launch requires a host-side mapper.
- VID/PID are not assigned in project source. The selected board/core configuration supplies defaults; do not present them as a project-owned USB identity.

The firmware uses the Arduino-Pico core's built-in Pico SDK USB stack and `Keyboard.h`. It handles volume up/down via encoder rotation, play/pause/next/previous via single/double/triple press, and sends Consumer Control AC Home (`0x0223`) on a 500 ms long-press for remapping by the head-unit keyboard remapper. The onboard RGB LED (GP16) blinks green on each emitted HID action.

## Legacy ESP8266 SWC material

Legacy project documentation and firmware are under [`firmware/esp8266-swc/`](firmware/esp8266-swc/README.md). Serial tooling remains in [`tools/`](tools/), and original logs remain in [`traces/`](traces/). They describe the old KEY1/KEY2 voltage-ladder system and must not be confused with native USB HID.

## Build/test status

RP2040 firmware was successfully tested on an Android head unit: all knob input patterns are fully functional, including the configured volume, media, and long-press actions. The firmware was built with Arduino CLI 1.5.1 + Arduino-Pico core 6.2.0 (`rp2040:rp2040:waveshare_rp2040_zero`). See the [firmware instructions](firmware/rp2040-zero/README.md).

## References

- [Waveshare RP2040-Zero](https://www.waveshare.com/wiki/RP2040-Zero)
- [Arduino-Pico USB documentation](https://arduino-pico.readthedocs.io/en/latest/usb.html)
- [USB HID screensaver research](HID-SCREENSAVER-RESEARCH.md) (observations and open verification items)
