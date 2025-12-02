# Developer Guide

Technical documentation for contributors and developers working on the ESP32-S3 Internet Monitor.

## Table of Contents

- [Build & Flash](#build--flash)
- [Architecture](#architecture)
- [Source Structure](#source-structure)
- [Module Dependencies](#module-dependencies)
- [State Machine](#state-machine)
- [Adding Features](#adding-features)
- [Web Server API](#web-server-api)
- [MQTT Integration](#mqtt-integration)
- [Security](#security)
- [Code Patterns](#code-patterns)
- [Hardware Notes](#hardware-notes)
- [UI Design System](#ui-design-system)

---

## Build & Flash

This is an Arduino IDE project (not PlatformIO).

### First-Time Setup

1. Arduino IDE → Tools → Board → **ESP32-S3-DevKitC**
2. Tools → Manage Libraries → Install:
   - `Adafruit NeoPixel`
   - `PubSubClient`
   - `ArduinoJson`
3. Open `InternetMonitor/InternetMonitor.ino`
4. (Optional) Edit `config.h` with WiFi credentials
5. Upload via USB-C

### Serial Monitor

- Baud rate: **115200**
- Enable "USB CDC On Boot" in Arduino IDE for serial output

### OTA Updates

After initial USB flash, update over WiFi:
- Port: `internet-monitor` (network)
- Password: `internet-monitor`

---

## Architecture

ESP32-S3 internet connectivity monitor with 8x8 WS2812B LED matrix. Displays connectivity status via colors/effects with a password-protected web dashboard.

### Dual-Core Design

| Core | Task | Description |
|------|------|-------------|
| Core 0 | LED effects | 60fps rendering, never blocks |
| Core 1 | Network | Internet checks (HTTP GET) |
| Core 1 | MQTT | Separate FreeRTOS task, non-blocking |
| Core 1 | Main loop | Web server, OTA handling |

The MQTT client runs in its own FreeRTOS task so connection attempts don't interfere with internet monitoring.

---

## Source Structure

```
InternetMonitor/
├── InternetMonitor.ino        # Main entry point, globals, setup/loop (~320 lines)
├── config.h                   # All configuration constants
│
├── core/                      # Core types and state management
│   ├── types.h                # Enums (State, Effect), structs (SystemStats, etc.)
│   └── state.h                # State machine, changeState(), helpers
│
├── storage/                   # Persistent storage
│   └── nvs_manager.h          # NVS read/write for credentials & settings
│
├── network/                   # Network operations
│   └── connectivity.h         # Internet checking logic (HTTP GET)
│
├── mqtt/                      # MQTT client and Home Assistant integration
│   ├── mqtt_config.h          # MQTTConfig struct, NVS persistence
│   ├── mqtt_manager.h         # PubSubClient wrapper, connection handling
│   ├── mqtt_payloads.h        # JSON status message builder
│   └── mqtt_ha_discovery.h    # Home Assistant auto-discovery messages
│
├── web/                       # Web server components
│   ├── auth.h                 # Session management, login/logout
│   ├── handlers.h             # Dashboard API handlers (/stats, /effect, etc.)
│   ├── mqtt_handlers.h        # MQTT config API handlers
│   ├── server.h               # Web server setup and route registration
│   ├── portal.h               # WiFi config portal logic
│   ├── ui_styles.h            # Shared CSS design system
│   ├── ui_modal.h             # Modal dialog component (HTML + JS)
│   ├── ui_login.h             # Login page HTML/CSS/JS
│   ├── ui_dashboard.h         # Dashboard HTML/CSS/JS
│   └── ui_portal.h            # Portal HTML/CSS/JS
│
├── system/                    # System services
│   ├── watchdog.h             # Watchdog timer setup
│   ├── ota.h                  # OTA update handlers
│   ├── tasks.h                # FreeRTOS task definitions (LED, Network)
│   └── factory_reset.h        # Hardware factory reset (BOOT button)
│
└── effects/                   # LED effects
    ├── effects_base.h         # Shared utilities, LUTs, effect config
    ├── effect_rain.h          # Individual effects...
    └── ...
```

---

## Module Dependencies

```
InternetMonitor.ino
    ├── config.h
    ├── core/types.h
    ├── effects.h → effects/effects_base.h → effects/effect_*.h
    ├── core/state.h → types.h, effects_base.h (setTargetColor)
    ├── storage/nvs_manager.h → types.h, config.h
    ├── network/connectivity.h → config.h
    ├── web/auth.h → types.h
    ├── web/handlers.h → state.h, nvs_manager.h, auth.h, ui_*.h
    ├── web/server.h → handlers.h, auth.h
    ├── web/portal.h → state.h, nvs_manager.h, auth.h, ui_*.h
    ├── system/*.h → config.h, state.h
    └── system/factory_reset.h → config.h, effects_base.h
```

---

## State Machine

Seven states with associated LED colors (defined in `config.h`):

| State | Color | Description |
|-------|-------|-------------|
| `STATE_BOOTING` | Blue | System starting |
| `STATE_CONNECTING_WIFI` | Cyan | WiFi connecting |
| `STATE_CONFIG_PORTAL` | Purple | AP mode, awaiting setup |
| `STATE_WIFI_LOST` | Red | WiFi disconnected |
| `STATE_INTERNET_OK` | Green | Internet reachable |
| `STATE_INTERNET_DEGRADED` | Yellow | 1 check failed |
| `STATE_INTERNET_DOWN` | Orange | 2+ consecutive failures |

State transitions are managed in `core/state.h` via `changeState()`.

---

## Adding Features

### Adding a New LED Effect

1. Create `effects/effect_yourname.h` with function:
   ```cpp
   void effectYourname() {
       // Your effect implementation
   }
   ```

2. Include in `effects.h`:
   ```cpp
   #include "effects/effect_yourname.h"
   ```

3. Add to `Effect` enum in `core/types.h`:
   ```cpp
   enum Effect {
       // ... existing effects
       EFFECT_YOURNAME,
       EFFECT_COUNT
   };
   ```

4. Add name to `effectNames[]` in `effects/effects_base.h`:
   ```cpp
   const char* effectNames[] = {
       // ... existing names
       "Your Name"
   };
   ```

5. Add case to `applyEffect()` switch in `effects.h`:
   ```cpp
   case EFFECT_YOURNAME: effectYourname(); break;
   ```

6. Add defaults to `effectDefaults[]` in `effects/effects_base.h`

### Adding a Web API Endpoint

1. Add handler function in `web/handlers.h`:
   ```cpp
   void handleYourEndpoint() {
       if (!isAuthenticated()) {
           server.send(401, "text/plain", "Unauthorized");
           return;
       }
       // Your logic here
       server.send(200, "application/json", "{\"status\":\"ok\"}");
   }
   ```

2. Register route in `web/server.h` → `setupWebServer()`:
   ```cpp
   server.on("/your-endpoint", HTTP_GET, handleYourEndpoint);
   ```

### Adding a New NVS Setting

1. Add key constant in `config.h`:
   ```cpp
   #define NVS_KEY_YOURSETTING "yoursetting"
   ```

2. Add load/save logic in `storage/nvs_manager.h`

---

## Web Server API

All endpoints except `/login` require session authentication.

### Core Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Dashboard (chunked response) |
| `/login` | POST | Authenticate (`password=xxx`) |
| `/logout` | GET | Clear session |
| `/stats` | GET | JSON statistics for live updates |
| `/effect?e={0-17}` | GET | Set LED effect |
| `/brightness?b={5-50}` | GET | Set brightness |
| `/speed?s={10-100}` | GET | Animation speed percentage |
| `/rotation?r={0-3}` | GET | Display rotation (0°/90°/180°/270°) |
| `/factory-reset` | GET | Clear all NVS and reboot |

### MQTT Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/mqtt/config` | GET | Get current MQTT configuration |
| `/mqtt/config` | POST | Save MQTT configuration |
| `/mqtt/status` | GET | Get MQTT connection status |
| `/mqtt/test` | POST | Test MQTT connection |
| `/mqtt/reset` | POST | Clear MQTT configuration |

---

## MQTT Integration

### Published Topics

| Topic | Description |
|-------|-------------|
| `{base_topic}/state` | JSON status payload (retained) |
| `{base_topic}/availability` | `online` or `offline` (retained, LWT) |

### JSON Payload Format

```json
{
  "status": "online",
  "state": 4,
  "state_text": "Online",
  "uptime_seconds": 86400,
  "total_checks": 8640,
  "success_rate": 99.8,
  "wifi_rssi": -52,
  "temperature": 42.5,
  "firmware": "0.7.0"
}
```

### Home Assistant Auto-Discovery

When enabled, publishes discovery messages to `homeassistant/sensor/internet_monitor/*/config` creating:

- Status (text sensor)
- Connectivity (binary sensor)
- Uptime, Success Rate, WiFi Signal, CPU Temperature
- Failed Checks, Total Downtime

---

## Security

### Password Storage

- Web password stored as **SHA-256 hash** in NVS (never plaintext)
- Uses ESP32's built-in `mbedtls` library
- Automatic migration from plaintext on first boot after upgrade

### Session Authentication

- Random 32-character tokens using hardware RNG (`esp_random()`)
- `HttpOnly` cookies prevent XSS
- `SameSite=Strict` prevents CSRF
- Rate limiting: 5 failed attempts → 1 minute lockout

### OTA Updates

- Fixed password: `internet-monitor`
- Watchdog disabled during update to prevent reset

### Config Portal (AP Mode)

- Fixed password `admin` during initial setup
- Only active when no WiFi credentials stored

---

## Code Patterns

### Modular Headers

Each concern lives in its own file with include guards:

```cpp
#ifndef MYMODULE_H
#define MYMODULE_H

// Implementation

#endif
```

### Extern Declarations

Headers declare externs, main `.ino` defines globals:

```cpp
// In header
extern volatile SystemStats stats;

// In .ino
volatile SystemStats stats;
```

### Cross-Core Shared State

Use `volatile` + mutex for data shared between cores:

```cpp
volatile State currentState = STATE_BOOTING;
SemaphoreHandle_t stateMutex = xSemaphoreCreateMutex();
```

### PROGMEM Strings

UI HTML stored in flash to save RAM:

```cpp
const char PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
...
)rawliteral";

// Usage
server.sendContent_P(PAGE_HTML);
```

### Chunked HTTP Responses

Dashboard served in chunks for memory efficiency:

```cpp
server.setContentLength(CONTENT_LENGTH_UNKNOWN);
server.send(200, "text/html", "");
server.sendContent("<html>...");
server.sendContent("</html>");
```

### Debounced NVS Writes

Settings changes batched with 3-second delay to reduce flash wear.

---

## Hardware Notes

| Component | Detail |
|-----------|--------|
| LED Data Pin | GPIO 14 |
| LED Count | 64 (8×8 matrix) |
| LED Type | WS2812B (NEO_RGB) |
| Max Brightness | 50 (thermal limit) |
| Watchdog Timeout | 60 seconds |
| WiFi | 2.4GHz only |

---

## UI Design System

All styling defined in `web/ui_styles.h`. Use these tokens for consistency.

### Color Palette

| Token | Hex | Usage |
|-------|-----|-------|
| `--bg-base` | `#0f0f1a` | Page background |
| `--bg-card` | `#1a1a2e` | Card/panel background |
| `--bg-input` | `#252540` | Input/button background |
| `--bg-hover` | `#303050` | Hover states |
| `--border` | `#252540` | Subtle borders |
| `--border-light` | `#303048` | Visible borders |
| `--text-primary` | `#c8c8d8` | Headings |
| `--text-secondary` | `#b8b8c8` | Body text |
| `--text-muted` | `#808098` | Secondary info |
| `--accent` | `#4338ca` | Primary actions |
| `--success` | `#22c55e` | Online, good |
| `--warning` | `#f59e0b` | Degraded, caution |
| `--error` | `#ef4444` | Offline, bad |
| `--danger` | `#b91c1c` | Destructive actions |

### CSS Classes

**Layout:**
- `.wrap` — Max-width container (420px mobile, 800px desktop)
- `.card` — Panel with background, border, padding
- `.card-title` — Uppercase label for sections
- `.grid` / `.grid-2` — 3-column / 2-column grid
- `.flex`, `.flex-between` — Flexbox helpers

**Buttons:**
- `.btn` — Standard button
- `.btn.active` — Active state (accent color)
- `.btn-danger` — Red destructive button
- `.btn-outline` — Transparent with border

**Stats Display:**
- `.stat` — Key-value row with border
- `.stat-label` / `.stat-val` — Left/right sides
- `.good` / `.warn` / `.bad` — Status colors

### Using Shared Styles

```cpp
#include "ui_styles.h"

void handlePage() {
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");
    server.sendContent("<html><head><style>");
    sendCommonStyles();  // Sends all base CSS
    server.sendContent("</style></head><body>");
    server.sendContent("<div class=\"card\">");
    server.sendContent("<span class=\"stat-val good\">Online</span>");
    server.sendContent("</div></body></html>");
}
```

### Modal Dialogs

Custom modals replace browser `alert()` and `confirm()`. Defined in `web/ui_modal.h`.

```cpp
#include "ui_modal.h"

// Before </body>:
server.sendContent(FPSTR(MODAL_HTML));

// In <script>:
server.sendContent(FPSTR(MODAL_JS));
```

**JavaScript API:**

```javascript
// Simple alerts
showAlert('Message');
showSuccess('Saved!', 'Success');
showError('Failed', 'Error');

// Confirmation
showConfirm('Are you sure?', {
    title: 'Confirm',
    confirmText: 'Yes',
    cancelText: 'No',
    danger: true,
    callback: function(confirmed) {
        if (confirmed) { /* proceed */ }
    }
});
```

### Toggle Switch Pattern

```cpp
// HTML
server.sendContent("<label class=\"tog\" onclick=\"myToggle()\">");
server.sendContent("<span class=\"tog-bg\" id=\"myBg\"></span>");
server.sendContent("<span class=\"tog-knob\" id=\"myKnob\"></span>");
server.sendContent("<input type=\"hidden\" id=\"myVal\" value=\"0\">");
server.sendContent("</label>");
```

```javascript
// JavaScript
function myToggle() {
    const inp = document.getElementById('myVal');
    const enabled = inp.value !== '1';
    inp.value = enabled ? '1' : '0';
    document.getElementById('myBg').style.background = enabled ? '#4338ca' : '#303048';
    document.getElementById('myKnob').style.left = enabled ? '22px' : '2px';
}
```
