# Web preview

Runs the firmware's real web handlers on a Linux host, with no board. Use it to
open the login page, the dashboard (with the MQTT tab) and the Wi-Fi setup
portal in a browser, and to take screenshots that show no real network details.

The HTML comes from `InternetMonitor/web/*.h` (`handleRoot`, `handlePortalRoot`,
`handleLogin`, the MQTT handlers) and the `ui_*.h` strings. Nothing is copied.

## Prerequisites

- `g++` (C++17) and `make`
- ArduinoJson in `~/Arduino/libraries/ArduinoJson` (override: `make ARDUINOJSON=<path to its src>`)
- For screenshots: Node, and Playwright with Chromium. `PLAYWRIGHT_DIR` is a
  directory that has `node_modules/playwright` (default `~/blog-esp32`).

## Run

```sh
make -C tools/web-preview shots     # build, start both previews, write out/*.png
make -C tools/web-preview run       # dashboard on http://127.0.0.1:8080/ (password: admin)
make -C tools/web-preview portal    # setup portal on http://127.0.0.1:8080/
```

`FW_DIR=<path>` builds against another copy of the `InternetMonitor` folder.
The server listens on loopback only.

## What is real and what is fake

Real: every route handler, the HTML and JSON they build, cookie login through
`handleLogin` (default password `admin`, SHA-256 checked), the MQTT config
handlers, `enterConfigMode()` for the portal, effect names and defaults.

Fake: all of `stubs/` (Arduino `String`, `WebServer`, `WiFi`, NVS as an
in-memory map, no LEDs, no MQTT broker, `ESP.restart()` does nothing), and the
device state in `fake_state.h` (about 3 days up, 26,104 checks, 52 failed,
48.3 C, effect Rain, MQTT connected with Home Assistant discovery off).
Wi-Fi values are invented: SSID `HomeNetwork`, IP `192.168.1.50`, a locally
administered MAC, and five made-up scan results.

State changes made in the browser stay in memory until the program stops.
