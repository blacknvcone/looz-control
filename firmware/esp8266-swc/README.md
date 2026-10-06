# Legacy firmware: NodeMCU / ESP8266 SWC ladder

This directory contains the preserved, legacy NodeMCU v3 / ESP8266 firmware and its original hardware/debug documentation. It is retained for reference; **the active target is RP2040 Zero USB HID** documented at the repository root and in `../rp2040-zero/`.

Original legacy documentation links: [plan](PLAN.md), [hardware](HARDWARE.md), [debug guide](DEBUG.md), [USB HID screensaver research](../../HID-SCREENSAVER-RESEARCH.md), and [original SWC wiring diagram](wiring.svg). The maintained RP2040 Zero encoder diagram is at [`../../docs/wiring-rp2040-zero-ky040.md`](../../docs/wiring-rp2040-zero-ky040.md).

The old design drives head-unit KEY1/KEY2 resistive-ladder inputs through PWM + RC and is unrelated to the new USB HID connection. Do not wire those analog outputs to RP2040 GPIOs.

Original source: `looz-control.ino`, `config.h`. Original design notes: `PLAN.md`, `HARDWARE.md`, `DEBUG.md`, and `wiring.svg`. The legacy sketch was previously built and exercised in its original environment as described in the historical root README; it has not been rebuilt in this environment.
