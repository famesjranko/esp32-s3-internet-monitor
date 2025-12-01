/*
 * ESP32-S3 Internet Monitor - DUAL CORE OPTIMIZED
 * 
 * Core 0: LED effects (smooth 60fps, never blocks)
 * Core 1: Network operations (WiFi, HTTP checks, MQTT, web server)
 * 
 * Features:
 * - Dual-core architecture for smooth LED animations
 * - Multiple fallback check URLs
 * - Consecutive failure threshold
 * - Watchdog timer (auto-reboot on hang)
 * - Smooth fade transitions
 * - Multiple LED effects
 * - Password protected web UI
 * - OTA updates
 * - Uptime/downtime tracking
 * - MQTT publishing with Home Assistant auto-discovery
 * 
 * Architecture: Modular design with separate concerns
 * - core/       : Types, enums, state management
 * - storage/    : NVS persistence
 * - network/    : Connectivity checking
 * - mqtt/       : MQTT client, payloads, HA discovery
 * - web/        : HTTP server, auth, handlers
 * - system/     : Watchdog, OTA, tasks
 * - effects/    : LED effects (unchanged)
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <Adafruit_NeoPixel.h>
#include <esp_task_wdt.h>
#include <PubSubClient.h>

#include "config.h"
#include "core/types.h"  // Enums and structs (must be early)
#include "mqtt/mqtt_config.h"  // MQTT configuration struct

// ===========================================
// GLOBAL INSTANCES
// ===========================================

Adafruit_NeoPixel pixels(NUM_LEDS, RGB_PIN, NEO_RGB + NEO_KHZ800);
WebServer server(80);
DNSServer dnsServer;
Preferences preferences;
HTTPClient httpClient;
bool httpClientInitialized = false;
MQTTConfig mqttConfig;  // MQTT configuration

// ===========================================
// TASK HANDLES
// ===========================================

TaskHandle_t ledTaskHandle = NULL;
TaskHandle_t networkTaskHandle = NULL;

// ===========================================
// MUTEX FOR THREAD SAFETY
// ===========================================

portMUX_TYPE stateMux = portMUX_INITIALIZER_UNLOCKED;

// ===========================================
// LOOKUP TABLES
// ===========================================

int8_t sinLUT[SIN_TABLE_SIZE];
bool lutInitialized = false;

// ===========================================
// VOLATILE STATE (cross-core access)
// ===========================================

volatile int currentState = STATE_BOOTING;
volatile int currentEffect = EFFECT_RAIN;  // Default effect on boot
volatile uint8_t currentBrightness = 10;
volatile uint8_t currentRotation = DEFAULT_ROTATION;
volatile uint8_t effectSpeed = 36;

// Colors for fading
volatile uint8_t currentR = 0, currentG = 0, currentB = 0;
volatile uint8_t targetR = 0, targetG = 0, targetB = 0;
uint8_t fadeStartR = 0, fadeStartG = 0, fadeStartB = 0;
unsigned long fadeStartTime = 0;

// Status flags
volatile bool isInternetOK = false;
volatile bool ledTaskRunning = true;
volatile bool ledTaskPaused = false;

// ===========================================
// STATIC DATA TABLES
// ===========================================

// Effect names for display
const char* effectNames[] = {
  "Off", "Solid", "Ripple", "Rainbow", "Rain",
  "Matrix", "Fire", "Plasma", "Ocean", "Nebula", "Life",
  "Pong", "Metaballs", "Interference", "Noise", "Pool", "Rings", "Ball"
};

// Per-effect default brightness and speed
// Format: {brightness, speed}
const uint8_t effectDefaults[][2] = {
  {5, 50},    // 0: Off (doesn't matter)
  {5, 50},    // 1: Solid (speed doesn't matter)
  {10, 72},   // 2: Ripple
  {5, 72},    // 3: Rainbow
  {10, 36},   // 4: Rain
  {5, 50},    // 5: Matrix
  {5, 51},    // 6: Fire
  {5, 100},   // 7: Plasma
  {5, 58},    // 8: Ocean
  {5, 58},    // 9: Nebula
  {5, 25},    // 10: Life
  {5, 36},    // 11: Pong
  {5, 100},   // 12: Metaballs
  {5, 50},    // 13: Interference
  {5, 84},    // 14: Noise
  {5, 80},    // 15: Pool
  {10, 57},   // 16: Rings
  {25, 57},   // 17: Ball
};

// ===========================================
// GLOBAL STATE STRUCTS
// ===========================================

SystemStats stats;
PerformanceMetrics perf;
AuthState auth;

// ===========================================
// TIMING & PORTAL STATE
// ===========================================

unsigned long lastCheck = 0;
unsigned long stateChangeTime = 0;
bool configPortalActive = false;
unsigned long lastPortalActivity = 0;
String cachedNetworkListHTML = "";

// ===========================================
// CREDENTIALS (loaded from NVS)
// ===========================================

String storedSSID = "";
String storedPassword = "";
String storedWebPasswordHash = "";  // SHA-256 hash, loaded from NVS

// ===========================================
// NVS DEBOUNCE
// ===========================================

bool settingsPendingSave = false;
unsigned long lastSettingChangeTime = 0;

// ===========================================
// INCLUDE MODULES (after globals are defined)
// ===========================================

// Effects (must be included before state.h due to setTargetColor)
#include "effects.h"

// Core state (needs effects_base.h for setTargetColor)
#include "core/state.h"
#include "core/crypto.h"

// Storage
#include "storage/nvs_manager.h"

// Network
#include "network/connectivity.h"

// Web modules
#include "web/auth.h"
#include "web/handlers.h"
#include "web/server.h"
#include "web/portal.h"

// System modules
#include "system/watchdog.h"
#include "system/ota.h"
#include "system/tasks.h"

// MQTT module (runs in its own task)
#include "mqtt/mqtt_manager.h"

// ===========================================
// SETUP
// ===========================================

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n\n=== Internet Monitor v" + String(FW_VERSION) + " ===");
  Serial.println("=== DUAL CORE ARCHITECTURE ===");
  Serial.printf("Main task running on Core %d\n", xPortGetCoreID());

  stats.bootTime = millis();

  // Initialize lookup tables for fast math in effects
  initLookupTables();
  Serial.println("Sin/Cos lookup tables initialized");

  // Init LEDs
  pixels.begin();
  
  // Load settings from NVS (brightness, effect, etc.)
  loadSettingsFromNVS();
  
  // Load MQTT configuration from NVS
  loadMQTTConfigFromNVS();

  // Apply loaded brightness
  pixels.setBrightness(currentBrightness);
  fillMatrixImmediate(COLOR_BOOTING_R, COLOR_BOOTING_G, COLOR_BOOTING_B);

  changeState(STATE_BOOTING);

  // Setup watchdog
  setupWatchdog();

  // Start LED task on Core 0 (runs independently for smooth animation)
  startLEDTask();

  // Try to load credentials from NVS
  if (!loadCredentialsFromNVS()) {
    // Fall back to config.h
    Serial.println("No NVS credentials, checking config.h fallback");
    storedSSID = WIFI_SSID;
    storedPassword = WIFI_PASSWORD;
  }
  
  // If web password is default ("admin"), check config.h for override
  if (storedWebPasswordHash == sha256("admin")) {
    // Check if config.h has a non-default password
    if (strlen(WEB_PASSWORD) > 0 && strcmp(WEB_PASSWORD, "admin") != 0) {
      storedWebPasswordHash = sha256(WEB_PASSWORD);
      Serial.println("Using config.h web password (hashed)");
    }
  }

  // Check if we have any credentials to use
  if (storedSSID.length() == 0) {
    Serial.println("No credentials available - entering config mode");
    enterConfigMode();
    return;
  }

  changeState(STATE_CONNECTING_WIFI);

  // Connect WiFi
  Serial.print("Connecting to ");
  Serial.println(storedSSID);

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(storedSSID.c_str(), storedPassword.c_str());

  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < WIFI_TIMEOUT) {
    delay(100);
    esp_task_wdt_reset();
    // LED effects continue running on Core 0!
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    setupWebServer();
    setupOTA();
    changeState(STATE_INTERNET_OK);
    
    // Start network monitoring task on Core 1
    startNetworkTask();
    
    // Start MQTT task on Core 1 (runs independently, won't block network checks)
    startMqttTask();
  } else {
    Serial.println("\nWiFi connection failed - entering config mode");
    enterConfigMode();
    return;
  }

  Serial.println("Setup complete!\n");
  Serial.println("Architecture:");
  Serial.println("  Core 0: LED effects @ 60fps");
  Serial.println("  Core 1: Network monitoring (independent task)");
  Serial.println("  Core 1: MQTT publishing (independent task)");
  Serial.println("  Main:   Web server + OTA");
  if (mqttConfig.enabled) {
    Serial.println("  MQTT:   " + String(mqttConfig.broker) + ":" + String(mqttConfig.port));
  }
}

// ===========================================
// MAIN LOOP (Core 1 - handles web server)
// ===========================================

void loop() {
  esp_task_wdt_reset();

  // Handle config portal mode
  if (configPortalActive) {
    dnsServer.processNextRequest();
    server.handleClient();

    // Check for portal timeout (10 min inactivity)
    if (millis() - lastPortalActivity > CONFIG_PORTAL_TIMEOUT) {
      Serial.println("Portal timeout - restarting");
      delay(1000);
      ESP.restart();
    }

    delay(10);
    return;  // Skip normal operation
  }

  // Save settings to NVS if needed (debounced)
  saveSettingsToNVSIfNeeded();

  // Handle OTA updates
  ArduinoOTA.handle();
  
  // Handle web server requests
  server.handleClient();

  // Small delay to prevent tight loop
  delay(5);
}
