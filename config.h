#ifndef CONFIG_H
#define CONFIG_H

// ===========================================
// WIFI CONFIGURATION
// ===========================================
const char* WIFI_SSID     = "";
const char* WIFI_PASSWORD = ""; 

// ===========================================
// WEB UI CONFIGURATION
// ===========================================
const char* WEB_PASSWORD  = "";
const char* FW_VERSION    = "0.7.1";

// ===========================================
// HARDWARE CONFIGURATION
// ===========================================
#define RGB_PIN           14
#define NUM_LEDS          64
#define MATRIX_SIZE       8
#define LED_BRIGHTNESS    40    // Max 50! Higher can damage board
#define BOOT_BUTTON_PIN   0     // GPIO0 - BOOT button on ESP32-S3-Matrix

// Hardware factory reset: Hold BOOT button during power-on
#define FACTORY_RESET_HOLD_TIME  5000  // Hold for 5 seconds to trigger reset

// Display rotation values (0-3)
#define ROTATION_0        0     // No rotation
#define ROTATION_90       1     // 90° clockwise
#define ROTATION_180      2     // 180°
#define ROTATION_270      3     // 270° clockwise

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
#define NVS_KEY_WEB_PASSWORD  "webpass"      // Legacy plaintext (migration)
#define NVS_KEY_WEB_PASS_HASH "webpass_hash" // SHA-256 hash
#define NVS_KEY_CONFIGURED    "configured"
#define NVS_KEY_BRIGHTNESS    "brightness"
#define NVS_KEY_EFFECT        "effect"
#define NVS_KEY_ROTATION      "rotation"
#define NVS_KEY_SPEED         "speed"

// MQTT NVS Keys
#define NVS_KEY_MQTT_ENABLED  "mqtt_en"
#define NVS_KEY_MQTT_BROKER   "mqtt_host"
#define NVS_KEY_MQTT_PORT     "mqtt_port"
#define NVS_KEY_MQTT_USER     "mqtt_user"
#define NVS_KEY_MQTT_PASS     "mqtt_pass"
#define NVS_KEY_MQTT_TOPIC    "mqtt_topic"
#define NVS_KEY_MQTT_INTERVAL "mqtt_int"
#define NVS_KEY_MQTT_HA_DISC  "mqtt_ha"

// ===========================================
// MQTT DEFAULTS
// ===========================================
#define MQTT_DEFAULT_PORT           1883
#define MQTT_DEFAULT_INTERVAL_MS    30000    // 30 seconds
#define MQTT_DEFAULT_TOPIC          "internet_monitor"
#define MQTT_RECONNECT_INTERVAL_MS  10000    // 10 seconds between reconnect attempts
#define MQTT_KEEPALIVE_SEC          60       // MQTT keepalive
#define MQTT_BUFFER_SIZE            768      // Message buffer size (HA discovery needs ~600)

// ===========================================
// DEFAULT SETTINGS (used on first boot / factory reset)
// ===========================================
// Note: Default effect is EFFECT_RAIN, defined in InternetMonitor.ino after the Effect enum
// Note: Brightness and speed defaults are per-effect, defined in effectDefaults[]
#define DEFAULT_ROTATION      2       // 0=0°, 1=90°, 2=180°, 3=270°

// ===========================================
// STATE COLORS (R, G, B)
// ===========================================
#define COLOR_BOOTING_R       0
#define COLOR_BOOTING_G       0
#define COLOR_BOOTING_B       80

#define COLOR_CONNECTING_R    0
#define COLOR_CONNECTING_G    40
#define COLOR_CONNECTING_B    80

#define COLOR_PORTAL_R        40
#define COLOR_PORTAL_G        0
#define COLOR_PORTAL_B        80

#define COLOR_WIFI_LOST_R     100
#define COLOR_WIFI_LOST_G     0
#define COLOR_WIFI_LOST_B     0

#define COLOR_OK_R            0
#define COLOR_OK_G            80
#define COLOR_OK_B            0

#define COLOR_DEGRADED_R      80
#define COLOR_DEGRADED_G      60
#define COLOR_DEGRADED_B      0

#define COLOR_DOWN_R          100
#define COLOR_DOWN_G          20
#define COLOR_DOWN_B          0

// ===========================================
// EFFECT PARAMETERS
// ===========================================
// Wave frequencies and amplitudes for effects
#define WAVE_FREQ_SLOW        0.4f
#define WAVE_FREQ_MEDIUM      0.7f
#define WAVE_FREQ_FAST        1.2f

// Brightness thresholds
#define BRIGHTNESS_MIN_RATIO  0.1f   // Minimum brightness in effects (10%)
#define BRIGHTNESS_MAX_RATIO  1.0f   // Maximum brightness in effects
#define HIGHLIGHT_THRESHOLD   0.75f  // When to add highlights (75%)

// Animation timing
#define ANIM_SPEED_DIVISOR    50.0f  // Base speed calculation divisor

// Fire effect parameters  
#define FIRE_COOLING_MIN      15
#define FIRE_COOLING_MAX      25
#define FIRE_COOLING_PER_ROW  6
#define FIRE_SPARK_CHANCE     100    // Out of 255
#define FIRE_HEAT_DECAY       0.9f   // 10% loss as heat rises

// Pool/water effect parameters
#define WATER_WAVE_FREQ       1.8f
#define WATER_HIGHLIGHT_THRESH 0.75f

// ===========================================
// LOOKUP TABLE SIZE
// ===========================================
#define SIN_TABLE_SIZE        256
#define FAST_SQRT_MAGIC       0x5f3759df  // Quake III fast inverse sqrt

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
