# Changelog

All notable changes to the ESP32-S3 Internet Monitor project, one entry per
merged pull request, newest first. Dates are the merge dates from the git
history. The repository has no tags or releases. The firmware version string
is the value of `FW_VERSION` in `config.h` after that pull request.

## PR #5: refactor directory, update readme, docs (2025-12-02)

Firmware version string: 0.7.1 (unchanged).

- Moved the sources into an `InternetMonitor/` directory (the `.ino` sketch, `config.h`, and the `effects/`, `mqtt/`, `network/`, `storage/`, `system/` and `web/` folders).
- Updated `README.md`.
- Added `docs/DEVELOPER_GUIDE.md`.

## PR #4: v3 improvements and fixes (2025-12-02)

Firmware version string: 0.7.1.

### Added
- **Hardware factory reset**: hold the BOOT button (GPIO0) for 5 seconds during normal operation to reset settings. Red rings fill inward as progress feedback.
- **MQTT reset**: a `/mqtt/reset` endpoint and dashboard control to clear the MQTT settings.
- **MQTT test**: a `/mqtt/test` endpoint used by the dashboard.

### Changed
- **Config portal**: the setup access point is now an open network, and the portal pages no longer require login.
- **MQTT defaults**: Home Assistant discovery now defaults to off.
- **Code organisation**: effect names and defaults and the factory-reset progress display moved to `effects/effects_base.h`. New `system/factory_reset.h` module.

### Fixed
- Long SSIDs no longer overflow the network-list buffer (200 to 400 bytes, with truncation of very long names).
- The portal scan buttons now pass the `event` object explicitly instead of relying on the deprecated global.

## PR #3: MQTT, modular refactor, performance and security (2025-12-02)

Firmware version string: 0.7.0.

### Added
- **MQTT support** (PubSubClient) in its own FreeRTOS task, with Home Assistant discovery payloads.
- **ArduinoJson** for JSON in the MQTT and web handlers.
- **Password hashing**: the web password is stored as a SHA-256 hash (mbedtls).
- **Modal dialogs** in the dashboard (`web/ui_modal.h`) and shared styles (`web/ui_styles.h`).
- **API error codes** (`APIError` in `core/types.h`) and rotation constants (`ROTATION_0` to `ROTATION_270`).
- `@file` documentation headers on source files.

### Changed
- The firmware was split into modules: `core/`, `mqtt/`, `network/`, `storage/`, `system/` and `web/`.
- The auth code in `web/auth.h` uses the hardware RNG (`esp_random()`).
- The OTA password is the fixed string `internet-monitor`, and the setup access point password is `admin`.

## PR #2: v2 improvements and fixes (2025-12-01)

Firmware version string: 0.6.0.

### Added
- **Modular effects system**: each effect in its own file under `effects/`, with shared helpers in `effects/effects_base.h`. The effect enum lists 18 entries including Off.
- **Dual-core operation**: LED effects run on Core 0 at about 60 fps, with a separate pinned network-check task. The dashboard shows FPS, frame time and task stack headroom.
- **Watchdog** timeout configured in `config.h` (`WDT_TIMEOUT`, 60 seconds).
- **Factory reset** button in the dashboard (`/factory-reset`).
- **Fast math**: a sine lookup table, `fastSinF()` and an inverse-square-root based `fastSqrt()`.
- **Pixel and utility helpers**: `setPixelAt()`, `clamp255()`, `lerpf()`, `mapFloat()`, `getScaledTime()` and `getTimeScale()`.
- Effect state colours (`COLOR_OK_*`, `COLOR_DOWN_*` and others) in `config.h`.
- `volatile` on cross-core shared variables, and a `portMUX_TYPE` spinlock in the state-change code.
- HTTP connection reuse (`setReuse(true)`) for the connectivity checks.

### Changed
- The web UI files moved to a `web/` directory, and a `web/ui_dashboard_extended.h` file was added.

Before this pull request, gamma correction and a serpentine matrix layout were
tried and reverted (the matrix stayed linear). The earlier changelog numbered
these attempts 0.5.0 to 0.5.2. They have no
commits of their own.

## PR #1: new WiFi portal, performance improvements (2025-11-27)

Firmware version string: 0.2.0 (0.1.0 before).

### Added
- **Config portal**: a captive portal (`InternetMonitor-Setup`, DNS redirect) for WiFi setup, with a portal page in `ui_portal.h`.
- **NVS persistence** (Preferences): WiFi credentials, brightness, effect, rotation and speed survive a reboot.
- A "Reset WiFi Settings" action (`/reset-wifi`) that clears the credentials and reboots into setup mode.

### Changed
- Settings writes to NVS are debounced (`NVS_WRITE_DELAY_MS`, 3 seconds) to reduce flash wear.
- The setup access point was protected with the web password (changed in PR #4).
- README rewritten for the setup flow.

## Initial commit (2025-11-26)

No firmware version string in the code at this commit.

- Internet connectivity monitor for the ESP32-S3 with an 8x8 WS2812B LED matrix.
- Redundant check URLs and a two-failure threshold before showing red.
- Web dashboard with a login, plus OTA updates.
- Five animated effects (Solid, Ripple, Rainbow, Pulse, Rain) and Off, described in the README.

## Version numbering

`FW_VERSION` is set by hand in `config.h`. Several numbers have no commit
of their own (0.3.0 to 0.5.3), so the string does not map one-to-one to
commits.
