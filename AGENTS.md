# AGENTS.md

Holds only what the code does not make obvious. Trust the code over this file; fix this file when they disagree.
Don't write history here; git, the PRs and `CHANGELOG.md` hold it.

## Build

Arduino sketch in `InternetMonitor/`. There is no test suite, so a clean compile is the only automated check:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32s3 InternetMonitor
```

Libraries: Adafruit NeoPixel, PubSubClient, ArduinoJson. Flash by USB-C the first time, then OTA.

A compile proves the change builds, nothing more. Say what was and was not checked on a board in every PR.

## Layout

`InternetMonitor.ino` includes header-only modules in a fixed order (see its `#include` list); a module may only use what is included before it.

- `config.h`: every tunable constant. Add new constants here, not inline.
- `core/`: state machine, shared types, SHA-256 helper.
- `network/`: the connectivity check.
- `effects/`: one `effect_<name>.h` per effect, plus `effects_base.h` (pixel helpers, fast math, state colour fade).
- `web/`: server, auth, handlers, and the UI as strings in `ui_*.h`.
- `mqtt/`, `storage/`, `system/`: MQTT and HA discovery, NVS, tasks, watchdog, OTA, factory reset.

`docs/DEVELOPER_GUIDE.md` has the detail.

## Rules with no mechanism

- **Cores.** The LED task owns core 0 and must never block. Network, web and MQTT work stays on core 1. State shared across cores is `volatile`, and state changes go through `changeState()` inside `stateMux`.
- **Timing budget.** A connectivity check (URLs tried × total timeout) must finish inside `CHECK_INTERVAL`, and every blocking call must reset the 60 s watchdog. Recompute the worst case when you change URLs, the cap or timeouts.
- **Effects show state.** Every effect except Off must take its colour from the fading state colour (`currentR/G/B`), so the display still reads as online, degraded or down. The steps to add an effect are in the header of `effects.h`.
- **This board only.** The Waveshare ESP32-S3-Matrix takes `NEO_RGB` order and is wired in plain rows. Gamma correction and a serpentine layout were tried and reverted; do not reintroduce them. Brightness is capped at 50 because the board overheats above it.
- **Flash is tight.** The sketch uses about 94% of program storage. Report the size from the compile output when a change adds code or UI.
- **NVS writes are debounced** (`NVS_WRITE_DELAY_MS`). Save settings through the NVS manager, never per slider step.
- **No secrets.** Keep `WIFI_SSID` and `WIFI_PASSWORD` empty in `config.h`; credentials come from the setup portal.

## Commits and PRs

- Conventional Commits prefixes: `fix:`, `feat:`, `docs:`. One concern per PR; describe the problem, then the solution, then how it was verified.
- A user-visible change adds an entry for its PR to `CHANGELOG.md` and bumps `FW_VERSION` in `config.h`.
- Never attribute work to an AI tool or model in commits, PRs or docs.
