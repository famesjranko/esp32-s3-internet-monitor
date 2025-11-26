# ESP32-S3 Internet Monitor

A visual internet connectivity monitor using the Waveshare ESP32-S3-Matrix. Displays real-time connection status through an 8x8 RGB LED matrix with animated effects, controllable via web interface.

![Web Interface](images/webui.png)

## Features

- **Real-time monitoring** — checks connectivity every 10 seconds
- **Visual status** — color-coded LED matrix shows connection state at a glance
- **6 LED effects** — Off, Solid, Ripple, Rainbow, Pulse, Rain
- **Web dashboard** — control effects, brightness, speed, and rotation
- **OTA updates** — update firmware over WiFi without USB
- **Watchdog timer** — auto-reboots if device hangs (60 second timeout)
- **False alarm prevention** — requires 2 consecutive failures before showing "down"

## Hardware

[Waveshare ESP32-S3-Matrix](https://www.waveshare.com/esp32-s3-matrix.htm)

![Hardware](images/hardware.jpg)

- ESP32-S3 dual-core @ 240MHz
- 8x8 WS2812B RGB LED matrix (64 LEDs)
- USB-C for power and programming
- WiFi 2.4GHz

> ⚠️ **Warning:** Keep brightness ≤50 to prevent overheating.

## Installation

### Prerequisites

1. [Arduino IDE 2.x](https://www.arduino.cc/en/software)
2. ESP32 board support — add this URL in File → Preferences → Additional Board URLs:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
   Then: Tools → Board → Boards Manager → search "esp32" → Install
3. **Adafruit NeoPixel** library — Tools → Manage Libraries → search "Adafruit NeoPixel" → Install

### Upload

1. Edit `InternetMonitor.ino` — set your WiFi credentials:
   ```cpp
   const char* WIFI_SSID     = "YourWiFiName";
   const char* WIFI_PASSWORD = "YourPassword";
   ```
2. Board settings in Tools menu:
   - Board: `ESP32S3 Dev Module`
   - USB CDC On Boot: `Enabled`
   - Port: Select your COM port
3. Click Upload

### First Boot

1. Blue LEDs while connecting to WiFi
2. Green when connected and internet OK
3. Check Serial Monitor (115200 baud) for IP address
4. Open IP in browser to access web interface

## LED Status Colors

| Color | Meaning |
|-------|---------|
| Blue | Booting / Connecting to WiFi |
| Green | Internet OK |
| Yellow | Degraded (1 check failed) |
| Orange | Internet down (2+ consecutive failures) |
| Red | WiFi disconnected |
| Purple | OTA update in progress |

## LED Effects

| Effect | Description |
|--------|-------------|
| Off | LEDs disabled |
| Solid | Static color, no animation |
| Ripple | Diagonal wave pattern |
| Rainbow | Flowing rainbow tinted by status color |
| Pulse | Breathing/fading animation |
| Rain | Falling droplets |

## Web Interface

Access via device IP address. Controls:

- **Effect buttons** — select animation
- **Brightness slider** — 5 to 50
- **Speed slider** — 10% to 100%
- **Rotation buttons** — 0°, 90°, 180°, 270°

Statistics shown:
- Uptime, total checks, success rate
- Failed checks, total downtime, last outage duration
- WiFi SSID, IP address, signal strength (RSSI)

## API Endpoints

| Endpoint | Description |
|----------|-------------|
| `GET /` | Web dashboard |
| `GET /stats` | JSON stats |
| `GET /effect?e={0-5}` | Set effect |
| `GET /brightness?b={5-50}` | Set brightness |
| `GET /speed?s={10-100}` | Set speed |
| `GET /rotation?r={0-3}` | Set rotation |

## OTA Updates

After initial USB upload, future updates can be done over WiFi:

1. In Arduino IDE: Tools → Port → Select `internet-monitor` (network)
2. Upload as normal

## Configuration

Edit these values in `InternetMonitor.ino`:

```cpp
#define CHECK_INTERVAL    10000  // Check every 10 seconds
#define WIFI_TIMEOUT      20000  // 20 seconds to connect
#define WDT_TIMEOUT       60     // Watchdog timeout (seconds)
#define FAILURES_BEFORE_RED  2   // Consecutive failures before "down"
```

Default settings: Rain effect, brightness 21, speed 80%, rotation 180°

## How It Works

1. Every 10 seconds, sends HTTP request to `clients3.google.com/generate_204`
2. HTTP 204 response = internet OK
3. Falls back to second URL if first fails
4. Updates LED color based on result
5. Tracks statistics (resets on reboot)

## Troubleshooting

**No Serial output:** Set USB CDC On Boot to Enabled

**Wrong colors:** Verify `NEO_RGB` in code (not `NEO_GRB`)

**Won't connect to WiFi:** ESP32 only supports 2.4GHz networks

**Crashes when internet down:** Watchdog should auto-recover within 60 seconds

## License

MIT
