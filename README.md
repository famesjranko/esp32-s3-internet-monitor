# ESP32-S3 Internet Monitor

An ESP32-S3 powered internet connectivity monitor featuring an 8x8 WS2812B RGB LED matrix. Continuously checks your connection and displays real-time status through color-coded animations — green when online, yellow when degraded, red when offline. Choose from 18 animated effects, control everything via a secure web dashboard, and update firmware over-the-air. Perfect for a desk, server room, or anywhere you want instant visual feedback on your internet health.

<p align="center">
  <img src="images/led_effects_gifs/rain_online.gif" width="150">
  <img src="images/led_effects_gifs/rain_degraded.gif" width="150">
  <img src="images/led_effects_gifs/rain_offline.gif" width="150">
  <br>
  <b>Online</b> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
  <b>Degraded</b> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
  <b>Offline</b>
</p>

## Features

- **At-a-glance status** — color-coded LED matrix shows connection state instantly
- **Real-time monitoring** — checks connectivity every 10 seconds
- **False alarm prevention** — requires 2 consecutive failures before showing "down"
- **Watchdog timer** — auto-reboots if device hangs (60 second timeout)
- **18 LED effects** — each with optimized default brightness and speed
- **WiFi provisioning** — configure WiFi via captive portal (no recompiling needed)
- **Persistent settings** — brightness, effect, speed, rotation saved to flash
- **Secure web dashboard** — session-based auth with rate-limited login
- **OTA updates** — update firmware over WiFi (uses same password as web UI)

## Hardware

[Waveshare ESP32-S3-Matrix](https://www.waveshare.com/esp32-s3-matrix.htm)

<img src="images/hardware.jpg" width="300">

- ESP32-S3 dual-core @ 240MHz
- 8x8 WS2812B RGB LED matrix (64 LEDs)
- USB-C for power and programming
- WiFi 2.4GHz

> ⚠️ **Warning:** Keep brightness ≤50 to prevent overheating.

## Quick Start

1. Set up Arduino IDE for ESP32-S3-Matrix: [Waveshare Wiki Guide](https://www.waveshare.com/wiki/ESP32-S3-Matrix#Working_with_Arduino)
2. Clone or download this repo, then **rename the folder to `InternetMonitor`** (Arduino requires the folder name to match the `.ino` filename)
3. Install **Adafruit NeoPixel** library via Library Manager
4. Edit `config.h` — set your web password (WiFi can be configured later via portal)
5. Upload to board
6. Connect to `InternetMonitor-Setup` WiFi network (password: your web password)
7. Go to `http://192.168.4.1`, login, and select your WiFi network
8. Device reboots and connects — check Serial Monitor (115200 baud) for IP address
9. Open IP in browser and login with your password

## How It Works

1. Every 10 seconds, sends HTTP request to `clients3.google.com/generate_204`
2. HTTP 204 response = internet OK
3. Falls back to secondary URL if first fails
4. Updates LED color based on result
5. Tracks statistics (resets on reboot)

## LED Display

### Status Colors

| Color | Meaning |
|-------|---------|
| 🟢 Green | Internet OK |
| 🟡 Yellow | Degraded (1 check failed) |
| 🔴 Red | Internet down (2+ consecutive failures) |
| 🔴 Red | WiFi disconnected |
| 🔵 Blue | Booting / Connecting to WiFi |
| 🟣 Purple | WiFi setup portal active |
| 🟪 Magenta | OTA update in progress |

### Effects

18 effects, each with optimized default brightness and speed:

| Effect | Brightness | Speed | Description |
|--------|------------|-------|-------------|
| Off | 5 | - | LEDs off |
| Solid | 5 | - | Static color |
| Ripple | 10 | 72 | Ripples from center |
| Rainbow | 5 | 72 | Rotating rainbow |
| Rain | 10 | 36 | Falling raindrops |
| Matrix | 5 | 50 | Falling code streams |
| Fire | 5 | 51 | Flickering flames |
| Plasma | 5 | 100 | Flowing plasma |
| Ocean | 5 | 58 | Ocean waves |
| Nebula | 5 | 58 | Space nebula |
| Life | 5 | 25 | Conway's Game of Life with tribes |
| Pong | 5 | 36 | Auto-playing Pong |
| Metaballs | 5 | 100 | Organic blobs |
| Interference | 5 | 50 | Wave interference |
| Noise | 5 | 84 | Perlin noise |
| Pool | 5 | 80 | Water ripples |
| Rings | 10 | 57 | Expanding rings |
| Ball | 25 | 57 | Bouncing ball |

When you switch effects, brightness and speed automatically adjust to the effect's defaults.

### Effect × Connectivity State

| Effect | Online | Degraded | Offline |
|:-------|:------:|:--------:|:-------:|
| **Solid** | <img src="images/led_effects_gifs/solid_online.gif" width="150"> | <img src="images/led_effects_gifs/solid_degraded.gif" width="150"> | <img src="images/led_effects_gifs/solid_offline.gif" width="150"> |
| **Ripple** | <img src="images/led_effects_gifs/ripple_online.gif" width="150"> | <img src="images/led_effects_gifs/ripple_degraded.gif" width="150"> | <img src="images/led_effects_gifs/ripple_offline.gif" width="150"> |
| **Rainbow** | <img src="images/led_effects_gifs/rainbow_online.gif" width="150"> | <img src="images/led_effects_gifs/rainbow_degraded.gif" width="150"> | <img src="images/led_effects_gifs/rainbow_offline.gif" width="150"> |
| **Rain** | <img src="images/led_effects_gifs/rain_online.gif" width="150"> | <img src="images/led_effects_gifs/rain_degraded.gif" width="150"> | <img src="images/led_effects_gifs/rain_offline.gif" width="150"> |

### System States

| Effect | Booting | Connecting | WiFi Lost | OTA |
|:-------|:------:|:--------:|:-------:|:---:|
| **Pulse** &nbsp;&nbsp;&nbsp;&nbsp;&thinsp; | <img src="images/led_effects_gifs/pulse_booting.gif" width="150"> | <img src="images/led_effects_gifs/pulse_connecting.gif" width="150"> | <img src="images/led_effects_gifs/pulse_wifi_lost.gif" width="150"> | <img src="images/led_effects_gifs/ota_progress.gif" width="150"> |

## WiFi Setup

Configure WiFi via the captive portal or hardcode credentials in `config.h` — your choice.

![WiFi Setup Portal](images/webgui-3.jpg)

**First-time setup:**
1. Device starts in setup mode (purple LEDs)
2. Connect to `InternetMonitor-Setup` WiFi (password: your web password from `config.h`)
3. Go to `http://192.168.4.1` and login
4. Select your WiFi network and enter password
5. Device reboots and connects

**Changing WiFi later:**
1. Open the web dashboard
2. Use Factory Reset in Danger Zone section
3. Device reboots into setup mode

Settings (brightness, effect, speed, rotation) are saved to flash and persist across reboots and WiFi changes.

## Web Interface

Access via device IP address. Sessions persist until device reboots or you logout. Rate limiting locks out after 5 failed login attempts (1 minute cooldown).

**Login**

![Login](images/webgui-1.jpg)

**Dashboard**

![Dashboard](images/webgui-2.jpg)

**Dashboard sections:**

- **Effects** — effect buttons with brightness, speed, and rotation controls
- **Statistics** — uptime, checks, success rate, downtime
- **Network** — SSID, IP, signal strength, MAC, channel
- **System** — architecture, memory, flash, temperature, OTA status, firmware version (collapsible)
- **Diagnostics** — LED FPS, frame times, stack usage (collapsible)
- **Danger Zone** — factory reset button

## Configuration

```
InternetMonitor/
├── InternetMonitor.ino   # Main logic, setup, loop
├── config.h              # Password, timing settings
├── effects.h             # Effect dispatcher
├── effects/              # Individual effect files (18 effects)
├── web/
│   ├── ui_login.h        # Login page HTML
│   ├── ui_dashboard.h    # Dashboard HTML/CSS/JS
│   └── ui_portal.h       # WiFi setup portal HTML/CSS/JS
├── CHANGELOG.md
└── README.md
```

Edit `config.h` to customize:

```cpp
// Password for web dashboard, WiFi setup portal, and OTA updates
const char* WEB_PASSWORD  = "admin";

// Timing
#define CHECK_INTERVAL       10000  // Check every 10 seconds
#define WDT_TIMEOUT          60     // Watchdog timeout (seconds)
#define FAILURES_BEFORE_RED  2      // Consecutive failures before "down"
```

WiFi credentials are configured via the setup portal and stored in flash.

Per-effect brightness and speed defaults are defined in `effectDefaults[]` array in the main `.ino` file.

## API Endpoints

All endpoints except `/login` require a valid session cookie.

| Endpoint | Description |
|----------|-------------|
| `GET /` | Web dashboard (or login page) |
| `POST /login` | Authenticate (body: `password=xxx`) |
| `GET /logout` | End session |
| `GET /stats` | JSON stats |
| `GET /effect?e={0-17}` | Set effect (returns JSON with new brightness/speed) |
| `GET /brightness?b={5-50}` | Set brightness |
| `GET /speed?s={10-100}` | Set speed |
| `GET /rotation?r={0-3}` | Set rotation |
| `GET /factory-reset` | Clear all settings and reboot |

## OTA Updates

After initial USB upload, future updates can be done over WiFi:

1. In Arduino IDE: Tools → Port → Select `internet-monitor at <ip>` (network port)
2. Enter password when prompted (same as web dashboard password, default: `admin`)
3. Upload as normal

The LED matrix shows purple progress bar during OTA upload.

> **Note:** If upload fails, try moving closer to router or upload via USB.

## Troubleshooting

**No Serial output:** Set USB CDC On Boot to Enabled

**Wrong colors:** Verify `NEO_RGB` in code (not `NEO_GRB`)

**Won't connect to WiFi:** ESP32 only supports 2.4GHz networks

**Crashes when internet down:** Watchdog should auto-recover within 60 seconds

**Captive portal not appearing:** Try opening http://192.168.4.1 manually

**OTA upload fails:** Check firewall, try USB upload instead

