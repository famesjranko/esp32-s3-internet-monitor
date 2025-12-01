# Changelog

All notable changes to the ESP32-S3 Internet Monitor project.

## [0.7.0] - 2024-01-XX

### Added
- **MQTT Support**: Publish status to MQTT broker for Home Assistant integration
  - Runs in dedicated FreeRTOS task (won't block network checks)
  - Configurable broker, port, username, password, topic
  - Home Assistant auto-discovery (8 entities)
  - Last Will Testament for offline detection
- **ArduinoJson**: Clean JSON serialization throughout codebase
- **Response Helpers**: `sendError()`, `sendSuccess()` for consistent API responses
- **Function Documentation**: Doxygen-style comments on all public APIs
- **UI Modal System**: Custom styled dialogs replacing browser alerts
- **Password Hashing**: Web password stored as SHA-256 hash (not plaintext)
  - Automatic migration from plaintext on first boot
  - Uses ESP32's built-in mbedtls library
- **API Error Codes**: `APIError` enum for consistent error handling
- **Rotation Constants**: `ROTATION_0`, `ROTATION_90`, `ROTATION_180`, `ROTATION_270`
- **File Headers**: @file documentation on all source files

### Changed
- **Effects Switch**: Now uses enum names (`EFFECT_RAIN`) instead of magic numbers
- **Token Generation**: Uses hardware RNG (`esp_random()`) for better security
- **Thread Safety**: `haDiscoveryPublished` now volatile for cross-core access
- **URL Check Loop**: Uses `min(2, numCheckUrls)` instead of hardcoded value
- **OTA Password**: Now fixed to "internet-monitor" for simplicity
- **AP Password**: Fixed to "admin" during config portal (was using web password)
- **API Responses**: All endpoints now return JSON (previously some returned "OK" text)

### Fixed
- MQTT buffer size increased to 768 bytes for HA discovery payloads
- Toggle switches in dashboard now update visually on click
- `ui_dashboard_extended.h` include guard was incorrectly `UI_DASHBOARD_H`

---

## [0.6.0] - 2024-01-XX

### Added
- **Thread Safety**: Added `volatile` keyword to all cross-core shared variables
- **Mutex Protection**: `changeState()` now uses `portMUX_TYPE` for thread-safe state transitions
- **Fast Math Library**: 
  - Sin/Cos lookup table (256 entries, initialized at startup)
  - `fastSinF()` / `fastCosF()` - ~4x faster than standard sin/cos
  - `fastSqrt()` using Quake III inverse sqrt algorithm (`0x5f3759df`)
  - `fastDist()` for distance calculations
- **Consistent Pixel API**: `setPixelAt(row, col, r, g, b)`, `setPixelRGB()`, `fillAll()`
- **Utility Functions**: `clamp255()`, `lerpf()`, `mapFloat()`, `getScaledTime()`, `getTimeScale()`
- **HTTP Connection Reuse**: Single `HTTPClient` instance with `setReuse(true)`
- **Effect Reset**: Effects now reset their state when switched to (ball position, noise phase, etc.)

### Changed
- **Named Constants**: All magic numbers replaced with `#define` constants in each effect file
- **State Colors**: Moved to `config.h` as `COLOR_OK_R/G/B`, `COLOR_DOWN_R/G/B`, etc.
- **File Organization**: Web UI files moved to `web/` directory
- **13 Effects Refactored**: Fire, Pool, Metaballs, Plasma, Nebula, Ocean, Interference, Noise, Pulse, Ripple, Rings, Ball, Solid

### Removed
- Unused `ledTaskIdleUs` and `netTaskIdleUs` variables

### Performance
- Effects using sin/sqrt now 3-4x faster due to lookup tables
- Reduced memory allocations in HTTP checks

---

## [0.5.3] - 2024-01-XX

### Fixed
- **Fire Effect**: Complete rewrite - now fills entire 8x8 matrix with animated flames
- **Pool Effect**: Removed red channel to prevent purple artifacts
- **Ocean Effect**: Fixed color order issue (was showing yellow/red instead of blue)
- **Pong Effect**: Ball now renders last so it's always visible over center line
- **Pulse Effect**: Smooth sine wave breathing instead of abrupt color changes
- **Nebula Effect**: Added color wave for purple/blue/pink shifting
- **Noise Effect**: Constrained brightness to prevent saturation
- **Interference Effect**: Distinct colors for constructive vs destructive interference
- **Metaballs Effect**: Adjusted formula to prevent solid color saturation
- **Game of Life**: Added stagnation detection (resets after 3 identical generations)

### Changed
- All effects now use `pixels.setPixelColor(i, r, g, b)` instead of `pixels.Color()` for consistency

---

## [0.5.2] - 2024-01-XX

### Fixed
- Removed gamma correction (was causing color issues)
- Fixed duplicate `#endif` in effect_solid.h
- Reverted to NEO_RGB color order (confirmed working)

---

## [0.5.1] - 2024-01-XX

### Fixed
- Reverted serpentine matrix layout (matrix is linear, not serpentine)
- Web password now correctly falls back to `config.h` if NVS is empty

---

## [0.5.0] - 2024-01-XX

### Added
- Attempted gamma correction (reverted in 0.5.2)
- Attempted serpentine layout support (reverted in 0.5.1)

---

## [0.4.0] - 2024-01-XX

### Added
- **Factory Reset**: Web UI button to clear all NVS settings and reboot to setup mode
- **Modular Effects System**: Each effect in its own file under `effects/` directory
- **19 LED Effects**: Off, Solid, Ripple, Rainbow, Pulse, Rain, Matrix, Fire, Plasma, Ocean, Nebula, Life, Pong, Metaballs, Interference, Noise, Pool, Rings, Ball

### Changed
- Effects organized into categories: Basic, Visual, Animated
- Dashboard groups effects by category

---

## [0.3.0] - 2024-01-XX

### Added
- **Dual-Core Architecture**: LED effects on Core 0, network on Core 1
- **60fps LED Rendering**: Smooth animations that never block
- **Performance Monitoring**: FPS, frame time, stack usage displayed in web UI
- **Watchdog Timer**: Auto-reboot if system hangs for 60 seconds

### Changed
- Web server runs on main loop (Core 1)
- Network checks run in dedicated FreeRTOS task

---

## [0.2.0] - 2024-01-XX

### Added
- **Config Portal**: Captive portal for WiFi setup when not configured
- **NVS Persistence**: Settings survive reboot (WiFi, brightness, effect, rotation, speed)
- **Multiple Check URLs**: Redundant connectivity checks (Google, Cloudflare)
- **Consecutive Failure Threshold**: Requires 2 failures before showing red

### Changed
- Password protected AP in config mode
- Debounced NVS writes to reduce flash wear

---

## [0.1.0] - 2024-01-XX

### Added
- Initial release
- Basic internet connectivity monitoring
- 8x8 WS2812B LED matrix support
- Web dashboard with login
- Brightness and rotation controls
- Basic LED effects
- OTA update support

---

## Version Numbering

- **Major** (X.0.0): Breaking changes or major new features
- **Minor** (0.X.0): New features, backward compatible
- **Patch** (0.0.X): Bug fixes, minor improvements
