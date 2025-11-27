#ifndef CONFIG_H
#define CONFIG_H

// ===========================================
// WIFI CONFIGURATION
// ===========================================
const char* WIFI_SSID     = ""; // Set this!
const char* WIFI_PASSWORD = ""; // Set this!

// ===========================================
// WEB UI CONFIGURATION
// ===========================================
const char* WEB_PASSWORD  = "admin";  // Change this!
const char* FW_VERSION    = "0.2.0";

// ===========================================
// HARDWARE CONFIGURATION
// ===========================================
#define RGB_PIN           14
#define NUM_LEDS          64
#define LED_BRIGHTNESS    30    // Max 50! Higher can damage board

// ===========================================
// TIMING CONFIGURATION (milliseconds)
// ===========================================
#define CHECK_INTERVAL    10000  // Check internet every 10 seconds
#define HTTP_TIMEOUT      5000   // 5 second timeout per request
#define WIFI_TIMEOUT      20000  // 20 seconds to connect
#define HEARTBEAT_INTERVAL 2000  // Pulse every 2 seconds
#define FADE_DURATION     500    // 500ms fade transitions
#define WDT_TIMEOUT       60     // Watchdog: reboot if stuck for 60 sec

// ===========================================
// FAILURE THRESHOLD
// ===========================================
#define FAILURES_BEFORE_RED  2   // Need 2 consecutive failures to show red

// ===========================================
// CONFIG PORTAL CONFIGURATION
// ===========================================
#define CONFIG_AP_SSID        "InternetMonitor-Setup"
#define CONFIG_AP_CHANNEL     1       // WiFi channel for AP
#define CONFIG_PORTAL_TIMEOUT 600000  // 10 min auto-exit from portal
#define NVS_WRITE_DELAY_MS    3000    // Debounce NVS writes (flash wear protection)

// ===========================================
// NVS STORAGE KEYS
// ===========================================
#define NVS_NAMESPACE         "imon"
#define NVS_KEY_SSID          "ssid"
#define NVS_KEY_PASSWORD      "password"
#define NVS_KEY_CONFIGURED    "configured"
#define NVS_KEY_BRIGHTNESS    "brightness"
#define NVS_KEY_EFFECT        "effect"
#define NVS_KEY_ROTATION      "rotation"
#define NVS_KEY_SPEED         "speed"

// ===========================================
// CHECK URLs (multiple for redundancy)
// ===========================================
const char* checkUrls[] = {
  "http://clients3.google.com/generate_204",
  "http://www.gstatic.com/generate_204",
  "http://connectivitycheck.gstatic.com/generate_204",
  "http://cp.cloudflare.com/"
};
const int numCheckUrls = 4;

#endif
