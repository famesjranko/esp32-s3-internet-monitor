/*
 * ESP32-S3 Internet Monitor - DUAL CORE OPTIMIZED
 * 
 * Core 0: LED effects (smooth 60fps, never blocks)
 * Core 1: Network operations (WiFi, HTTP checks, web server)
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
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <Adafruit_NeoPixel.h>
#include <esp_task_wdt.h>

#include "config.h"
#include "web/ui_login.h"
#include "web/ui_dashboard.h"
#include "web/ui_portal.h"

// ===========================================
// DUAL CORE CONFIGURATION
// ===========================================

#define LED_CORE 0          // Core for LED effects (smooth animation)
#define NETWORK_CORE 1      // Core for network operations
#define LED_TASK_PRIORITY 2 // Higher priority for smooth animation
#define NET_TASK_PRIORITY 1 // Lower priority for network

TaskHandle_t ledTaskHandle = NULL;
TaskHandle_t networkTaskHandle = NULL;

// ===========================================
// STATE MACHINE
// ===========================================

enum State {
  STATE_BOOTING = 0,
  STATE_CONNECTING_WIFI = 1,
  STATE_CONFIG_PORTAL = 2,
  STATE_WIFI_LOST = 3,
  STATE_INTERNET_OK = 4,
  STATE_INTERNET_DEGRADED = 5,
  STATE_INTERNET_DOWN = 6
};

enum Effect {
  EFFECT_OFF,
  EFFECT_SOLID,
  EFFECT_RIPPLE,
  EFFECT_RAINBOW,
  EFFECT_RAIN,
  EFFECT_MATRIX,
  EFFECT_FIRE,
  EFFECT_PLASMA,
  EFFECT_OCEAN,
  EFFECT_NEBULA,
  EFFECT_LIFE,
  EFFECT_PONG,
  EFFECT_METABALLS,
  EFFECT_INTERFERENCE,
  EFFECT_NOISE,
  EFFECT_RIPPLE_POOL,
  EFFECT_RINGS,
  EFFECT_BALL,
  NUM_EFFECTS  // = 18
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
// GLOBALS (volatile for cross-core access)
// ===========================================

Adafruit_NeoPixel pixels(NUM_LEDS, RGB_PIN, NEO_RGB + NEO_KHZ800);
WebServer server(80);

// Mutex for thread-safe state changes
portMUX_TYPE stateMux = portMUX_INITIALIZER_UNLOCKED;

// Lookup tables for fast math (defined here, used in effects)
int8_t sinLUT[SIN_TABLE_SIZE];
bool lutInitialized = false;

// State - volatile for cross-core access
volatile int currentState = STATE_BOOTING;
volatile int currentEffect = EFFECT_RAIN;  // Default effect on boot
const char* effectNames[] = {
  "Off", "Solid", "Ripple", "Rainbow", "Rain",
  "Matrix", "Fire", "Plasma", "Ocean", "Nebula", "Life",
  "Pong", "Metaballs", "Interference", "Noise", "Pool", "Rings", "Ball"
};
// Brightness and speed initialized from per-effect defaults in setup()
volatile uint8_t currentBrightness = 10;
volatile uint8_t currentRotation = DEFAULT_ROTATION;
volatile uint8_t effectSpeed = 36;

// Colors for fading - volatile for cross-core access
volatile uint8_t currentR = 0, currentG = 0, currentB = 0;
volatile uint8_t targetR = 0, targetG = 0, targetB = 0;
unsigned long fadeStartTime = 0;
uint8_t fadeStartR = 0, fadeStartG = 0, fadeStartB = 0;

// Status flag for effects - volatile for cross-core access
volatile bool isInternetOK = false;

// LED task control
volatile bool ledTaskRunning = true;
volatile bool ledTaskPaused = false;

// Timing
unsigned long lastCheck = 0;
unsigned long stateChangeTime = 0;
unsigned long bootTime = 0;

// Session management
String sessionToken = "";

// Rate limiting
int loginAttempts = 0;
unsigned long lockoutUntil = 0;
const int MAX_LOGIN_ATTEMPTS = 5;
const unsigned long LOCKOUT_DURATION = 60000;  // 1 minute

// Stats
unsigned long totalChecks = 0;
unsigned long successfulChecks = 0;
unsigned long failedChecks = 0;
int consecutiveFailures = 0;
int consecutiveSuccesses = 0;
unsigned long lastDowntime = 0;
unsigned long totalDowntimeMs = 0;
unsigned long downtimeStart = 0;
bool wasDown = false;

// Config portal
DNSServer dnsServer;
Preferences preferences;
bool configPortalActive = false;
unsigned long lastPortalActivity = 0;
String cachedNetworkListHTML = "";

// Stored credentials (loaded from NVS or config.h fallback)
String storedSSID = "";
String storedPassword = "";
String storedWebPassword = "admin";  // Default password

// NVS settings debounce
bool settingsPendingSave = false;
unsigned long lastSettingChangeTime = 0;

// ===========================================
// PERFORMANCE MONITORING
// ===========================================

// FPS tracking
volatile unsigned long ledFrameCount = 0;
volatile float ledActualFPS = 60.0;
volatile unsigned long ledFrameTimeUs = 0;
volatile unsigned long ledMaxFrameTimeUs = 0;

// Task stack monitoring
volatile unsigned long ledStackHighWater = 0;
volatile unsigned long netStackHighWater = 0;

// Performance logging interval
#define PERF_LOG_INTERVAL 5000  // Log every 5 seconds

// ===========================================
// REUSABLE HTTP CLIENT
// ===========================================
HTTPClient httpClient;
bool httpClientInitialized = false;

// Include effects after globals are defined
#include "effects.h"

// ===========================================
// LED TASK (Core 0) - Smooth 60fps animation
// ===========================================

void ledTask(void* parameter) {
  const TickType_t frameDelay = pdMS_TO_TICKS(16);  // ~60fps (16.67ms per frame)
  TickType_t lastWakeTime = xTaskGetTickCount();
  
  // FPS tracking variables
  unsigned long frameCount = 0;
  unsigned long lastFPSReport = millis();
  unsigned long frameStartUs;
  unsigned long maxFrameUs = 0;
  
  Serial.println("[LED Task] Started on Core " + String(xPortGetCoreID()));
  
  // Add this task to watchdog
  esp_task_wdt_add(NULL);
  
  while (ledTaskRunning) {
    esp_task_wdt_reset();
    frameStartUs = micros();
    
    if (!ledTaskPaused) {
      // Update fade and apply effect
      updateFade();
      applyEffect();
    }
    
    // Measure frame time
    unsigned long frameUs = micros() - frameStartUs;
    ledFrameTimeUs = frameUs;
    if (frameUs > maxFrameUs) maxFrameUs = frameUs;
    
    frameCount++;
    ledFrameCount++;
    
    // Report FPS every 5 seconds
    unsigned long now = millis();
    if (now - lastFPSReport >= PERF_LOG_INTERVAL) {
      float fps = (float)frameCount * 1000.0 / (now - lastFPSReport);
      ledActualFPS = fps;
      ledMaxFrameTimeUs = maxFrameUs;
      ledStackHighWater = uxTaskGetStackHighWaterMark(NULL);
      
      Serial.printf("[LED] FPS: %.1f | Frame: %lu us (max %lu us) | Stack: %lu bytes free\n", 
        fps, ledFrameTimeUs, maxFrameUs, ledStackHighWater * 4);
      
      frameCount = 0;
      maxFrameUs = 0;
      lastFPSReport = now;
    }
    
    // Use vTaskDelayUntil for precise timing
    vTaskDelayUntil(&lastWakeTime, frameDelay);
  }
  
  esp_task_wdt_delete(NULL);
  vTaskDelete(NULL);
}

// ===========================================
// NETWORK TASK (Core 1) - Internet checking
// ===========================================

void networkTask(void* parameter) {
  const TickType_t checkDelay = pdMS_TO_TICKS(100);  // Check every 100ms
  
  // Performance tracking
  unsigned long lastPerfReport = millis();
  unsigned long checkCount = 0;
  unsigned long totalCheckTimeMs = 0;
  
  Serial.println("[Network Task] Started on Core " + String(xPortGetCoreID()));
  
  // Add this task to watchdog
  esp_task_wdt_add(NULL);
  
  while (true) {
    esp_task_wdt_reset();
    
    // Update stack high water mark periodically
    unsigned long now = millis();
    if (now - lastPerfReport >= PERF_LOG_INTERVAL) {
      netStackHighWater = uxTaskGetStackHighWaterMark(NULL);
      
      if (checkCount > 0) {
        Serial.printf("[Net] Checks: %lu | Avg time: %lu ms | Stack: %lu bytes free\n",
          checkCount, totalCheckTimeMs / checkCount, netStackHighWater * 4);
      }
      
      checkCount = 0;
      totalCheckTimeMs = 0;
      lastPerfReport = now;
    }
    
    // Skip if in config portal mode
    if (configPortalActive) {
      vTaskDelay(checkDelay);
      continue;
    }
    
    // Check WiFi status
    if (WiFi.status() != WL_CONNECTED) {
      if (currentState != STATE_WIFI_LOST && currentState != STATE_CONNECTING_WIFI && 
          currentState != STATE_CONFIG_PORTAL && currentState != STATE_BOOTING) {
        Serial.println("[Network] WiFi lost!");
        changeState(STATE_WIFI_LOST);
      }
      vTaskDelay(checkDelay);
      continue;
    }
    
    // WiFi recovered
    if (currentState == STATE_WIFI_LOST) {
      Serial.println("[Network] WiFi recovered");
      changeState(STATE_INTERNET_OK);
    }
    
    // Internet check (non-blocking timing)
    now = millis();
    if (now - lastCheck >= CHECK_INTERVAL) {
      lastCheck = now;
      
      unsigned long checkStart = millis();
      Serial.print("[Network] Checking... ");
      int successes = checkInternet();
      unsigned long checkTime = millis() - checkStart;
      
      checkCount++;
      totalCheckTimeMs += checkTime;
      
      totalChecks++;
      
      if (successes >= 1) {
        Serial.printf("OK (%lu ms)\n", checkTime);
        successfulChecks++;
        consecutiveFailures = 0;
        consecutiveSuccesses++;
        
        if (currentState != STATE_INTERNET_OK) {
          changeState(STATE_INTERNET_OK);
        }
      } else {
        Serial.printf("FAIL (%lu ms)\n", checkTime);
        failedChecks++;
        consecutiveFailures++;
        consecutiveSuccesses = 0;
        
        if (consecutiveFailures >= FAILURES_BEFORE_RED) {
          changeState(STATE_INTERNET_DOWN);
        } else {
          changeState(STATE_INTERNET_DEGRADED);
        }
      }
    }
    
    vTaskDelay(checkDelay);
  }
}

// ===========================================
// INTERNET CHECK (reuses HTTP client)
// ===========================================

bool checkSingleUrl(const char* url) {
  // Reuse the global HTTP client for connection pooling
  httpClient.setConnectTimeout(2000);
  httpClient.setTimeout(3000);
  httpClient.setReuse(true);  // Enable connection reuse
  
  if (!httpClient.begin(url)) {
    return false;
  }
  
  int code = httpClient.GET();
  httpClient.end();
  return (code == 204 || code == 200);
}

int checkInternet() {
  int successes = 0;
  
  // Try up to 2 URLs, return early on first success
  for (int i = 0; i < 2; i++) {
    esp_task_wdt_reset();
    
    if (checkSingleUrl(checkUrls[i])) {
      successes++;
      esp_task_wdt_reset();
      return successes;  // Early return on success
    }
    
    esp_task_wdt_reset();
  }
  
  return successes;
}

// ===========================================
// STATE MANAGEMENT (thread-safe with mutex)
// ===========================================

void changeState(State newState) {
  if (currentState == newState) return;
  
  // Use mutex for thread-safe state changes
  portENTER_CRITICAL(&stateMux);
  
  Serial.print("[State] ");
  Serial.print(currentState);
  Serial.print(" -> ");
  Serial.println(newState);
  
  // Track downtime
  if (newState == STATE_INTERNET_DOWN && !wasDown) {
    downtimeStart = millis();
    wasDown = true;
  } else if (newState == STATE_INTERNET_OK && wasDown) {
    totalDowntimeMs += millis() - downtimeStart;
    lastDowntime = millis() - downtimeStart;
    wasDown = false;
  }
  
  currentState = newState;
  stateChangeTime = millis();
  isInternetOK = (newState == STATE_INTERNET_OK);

  portEXIT_CRITICAL(&stateMux);

  // Set colors based on state (using config.h constants)
  switch (newState) {
    case STATE_BOOTING:
      setTargetColor(COLOR_BOOTING_R, COLOR_BOOTING_G, COLOR_BOOTING_B);
      break;
    case STATE_CONNECTING_WIFI:
      setTargetColor(COLOR_CONNECTING_R, COLOR_CONNECTING_G, COLOR_CONNECTING_B);
      break;
    case STATE_CONFIG_PORTAL:
      setTargetColor(COLOR_PORTAL_R, COLOR_PORTAL_G, COLOR_PORTAL_B);
      break;
    case STATE_WIFI_LOST:
      setTargetColor(COLOR_WIFI_LOST_R, COLOR_WIFI_LOST_G, COLOR_WIFI_LOST_B);
      break;
    case STATE_INTERNET_OK:
      setTargetColor(COLOR_OK_R, COLOR_OK_G, COLOR_OK_B);
      break;
    case STATE_INTERNET_DEGRADED:
      setTargetColor(COLOR_DEGRADED_R, COLOR_DEGRADED_G, COLOR_DEGRADED_B);
      break;
    case STATE_INTERNET_DOWN:
      setTargetColor(COLOR_DOWN_R, COLOR_DOWN_G, COLOR_DOWN_B);
      break;
  }
}

// ===========================================
// HELPERS
// ===========================================

String formatUptime(unsigned long ms) {
  unsigned long seconds = ms / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  unsigned long days = hours / 24;
  
  String result = "";
  if (days > 0) result += String(days) + "d ";
  if (hours % 24 > 0) result += String(hours % 24) + "h ";
  if (minutes % 60 > 0) result += String(minutes % 60) + "m ";
  result += String(seconds % 60) + "s";
  return result;
}

// ===========================================
// AUTH & TOKENS
// ===========================================

String generateToken() {
  String token = "";
  const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  for (int i = 0; i < 32; i++) {
    token += chars[random(0, 62)];
  }
  return token;
}

bool checkAuth() {
  // Check session token in cookie or header
  if (server.hasHeader("Cookie")) {
    String cookie = server.header("Cookie");
    int idx = cookie.indexOf("session=");
    if (idx >= 0) {
      String token = cookie.substring(idx + 8);
      int end = token.indexOf(';');
      if (end > 0) token = token.substring(0, end);
      if (token.length() > 0 && token == sessionToken) {
        return true;
      }
    }
  }
  // Also check Authorization header for API calls
  if (server.hasHeader("Authorization")) {
    String auth = server.header("Authorization");
    String token = auth.substring(7);
    if (token.length() > 0 && token == sessionToken) {
      return true;
    }
  }
  return false;
}

bool isLockedOut() {
  if (millis() < lockoutUntil) {
    return true;
  }
  if (millis() >= lockoutUntil && loginAttempts >= MAX_LOGIN_ATTEMPTS) {
    loginAttempts = 0;  // Reset after lockout expires
  }
  return false;
}

void sendUnauthorized() {
  server.send(401, "application/json", "{\"error\":\"unauthorized\"}");
}

float getChipTemp() {
  return temperatureRead();
}

// ===========================================
// NVS CREDENTIAL MANAGEMENT
// ===========================================

bool loadCredentialsFromNVS() {
  preferences.begin(NVS_NAMESPACE, true);  // read-only
  bool configured = preferences.getBool(NVS_KEY_CONFIGURED, false);
  if (configured) {
    storedSSID = preferences.getString(NVS_KEY_SSID, "");
    storedPassword = preferences.getString(NVS_KEY_PASSWORD, "");
  }
  // Always try to load web password (may exist even if WiFi not configured)
  storedWebPassword = preferences.getString(NVS_KEY_WEB_PASSWORD, "admin");
  preferences.end();

  Serial.print("NVS configured: ");
  Serial.println(configured ? "yes" : "no");
  if (configured && storedSSID.length() > 0) {
    Serial.print("NVS SSID: ");
    Serial.println(storedSSID);
  }

  return configured && storedSSID.length() > 0;
}

void saveCredentialsToNVS(const String& ssid, const String& password) {
  preferences.begin(NVS_NAMESPACE, false);  // read-write
  preferences.putString(NVS_KEY_SSID, ssid);
  preferences.putString(NVS_KEY_PASSWORD, password);
  preferences.putBool(NVS_KEY_CONFIGURED, true);
  preferences.end();

  Serial.println("Credentials saved to NVS");
}

void clearNVSCredentials() {
  preferences.begin(NVS_NAMESPACE, false);
  preferences.remove(NVS_KEY_SSID);
  preferences.remove(NVS_KEY_PASSWORD);
  preferences.putBool(NVS_KEY_CONFIGURED, false);
  preferences.end();
  Serial.println("NVS credentials cleared");
}

// ===========================================
// NVS SETTINGS PERSISTENCE
// ===========================================

void loadSettingsFromNVS() {
  preferences.begin(NVS_NAMESPACE, true);  // read-only

  // Load effect first (needed to get per-effect defaults)
  if (preferences.isKey(NVS_KEY_EFFECT)) {
    currentEffect = preferences.getUChar(NVS_KEY_EFFECT, currentEffect);
  }
  
  // Apply per-effect defaults, then override with saved values if they exist
  if (currentEffect < NUM_EFFECTS) {
    currentBrightness = effectDefaults[currentEffect][0];
    effectSpeed = effectDefaults[currentEffect][1];
  }
  
  // Override with saved values if they exist
  if (preferences.isKey(NVS_KEY_BRIGHTNESS)) {
    currentBrightness = preferences.getUChar(NVS_KEY_BRIGHTNESS, currentBrightness);
  }
  if (preferences.isKey(NVS_KEY_ROTATION)) {
    currentRotation = preferences.getUChar(NVS_KEY_ROTATION, currentRotation);
  }
  if (preferences.isKey(NVS_KEY_SPEED)) {
    effectSpeed = preferences.getUChar(NVS_KEY_SPEED, effectSpeed);
  }

  preferences.end();

  Serial.print("Loaded settings - Brightness: ");
  Serial.print(currentBrightness);
  Serial.print(", Effect: ");
  Serial.print(currentEffect);
  Serial.print(", Rotation: ");
  Serial.print(currentRotation);
  Serial.print(", Speed: ");
  Serial.println(effectSpeed);
}

void markSettingsChanged() {
  settingsPendingSave = true;
  lastSettingChangeTime = millis();
}

void saveSettingsToNVSIfNeeded() {
  // Only save if settings changed and debounce time has passed
  if (!settingsPendingSave) return;
  if (millis() - lastSettingChangeTime < NVS_WRITE_DELAY_MS) return;

  preferences.begin(NVS_NAMESPACE, false);  // read-write
  preferences.putUChar(NVS_KEY_BRIGHTNESS, currentBrightness);
  preferences.putUChar(NVS_KEY_EFFECT, currentEffect);
  preferences.putUChar(NVS_KEY_ROTATION, currentRotation);
  preferences.putUChar(NVS_KEY_SPEED, effectSpeed);
  preferences.end();

  settingsPendingSave = false;
  Serial.println("Settings saved to NVS");
}

// ===========================================
// WEB HANDLERS
// ===========================================

void handleLogin() {
  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"error\":\"method not allowed\"}");
    return;
  }

  if (isLockedOut()) {
    unsigned long remaining = (lockoutUntil - millis()) / 1000;
    server.send(429, "application/json", "{\"error\":\"too many attempts\",\"retry_after\":" + String(remaining) + "}");
    return;
  }

  String password = server.arg("password");

  if (password == storedWebPassword) {
    loginAttempts = 0;
    sessionToken = generateToken();

    // Max-Age=31536000 = 1 year (browser will remember)
    server.sendHeader("Set-Cookie", "session=" + sessionToken + "; Path=/; HttpOnly; SameSite=Strict; Max-Age=31536000");
    server.send(200, "application/json", "{\"success\":true,\"token\":\"" + sessionToken + "\"}");
  } else {
    loginAttempts++;
    if (loginAttempts >= MAX_LOGIN_ATTEMPTS) {
      lockoutUntil = millis() + LOCKOUT_DURATION;
    }
    server.send(401, "application/json", "{\"error\":\"invalid password\",\"attempts\":" + String(MAX_LOGIN_ATTEMPTS - loginAttempts) + "}");
  }
}

void handleLogout() {
  sessionToken = "";
  server.sendHeader("Set-Cookie", "session=; Path=/; HttpOnly; Max-Age=0");
  server.send(200, "application/json", "{\"success\":true}");
}

void handleRoot() {
  if (!checkAuth()) {
    server.send(200, "text/html", LOGIN_HTML);
    return;
  }

  // Build dashboard using chunked response to reduce memory fragmentation
  unsigned long uptime = millis() - bootTime;
  float successRate = totalChecks > 0 ? (100.0 * successfulChecks / totalChecks) : 100;

  String stateStr, stateColor;
  switch (currentState) {
    case STATE_INTERNET_OK: stateStr = "ONLINE"; stateColor = "#22c55e"; break;
    case STATE_INTERNET_DEGRADED: stateStr = "DEGRADED"; stateColor = "#f59e0b"; break;
    case STATE_INTERNET_DOWN: stateStr = "OFFLINE"; stateColor = "#ef4444"; break;
    case STATE_WIFI_LOST: stateStr = "NO WIFI"; stateColor = "#ef4444"; break;
    default: stateStr = "STARTING"; stateColor = "#3b82f6"; break;
  }

  // Use chunked transfer to reduce memory fragmentation
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  // Head
  server.sendContent("<!DOCTYPE html><html><head>");
  server.sendContent("<meta charset=\"UTF-8\">");
  server.sendContent("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  server.sendContent("<title>Internet Monitor</title><style>");
  server.sendContent(FPSTR(DASHBOARD_CSS));
  server.sendContent("</style></head><body><div class=\"wrap\">");

  // Header
  server.sendContent("<div class=\"hdr\"><div class=\"hdr-left\">");
  server.sendContent("<h1>Internet Monitor</h1>");
  server.sendContent("<p class=\"sub\">ESP32-S3 MATRIX • DUAL CORE</p>");
  server.sendContent("</div><button class=\"logout\" onclick=\"logout()\">Logout</button></div>");

  // Status
  server.sendContent("<div class=\"status\">");
  server.sendContent("<span class=\"status-dot\" id=\"dot\" style=\"background:" + stateColor + ";box-shadow:0 0 8px " + stateColor + "\"></span>");
  server.sendContent("<span class=\"status-text\" id=\"stxt\" style=\"color:" + stateColor + "\">" + stateStr + "</span>");
  server.sendContent("</div>");

  // Effects card - collapsible, all effects in one grid
  server.sendContent("<div class=\"card\"><div class=\"card-title collapsible\" id=\"effectsT\" onclick=\"T('effects')\"><span>Effects</span><span class=\"toggle\">▼</span></div>");
  server.sendContent("<div class=\"card-body\" id=\"effectsB\"><div class=\"grid\">");
  server.sendContent("<button class=\"btn off" + String(currentEffect == 0 ? " active" : "") + "\" onclick=\"E(0)\">Off</button>");
  server.sendContent("<button class=\"btn" + String(currentEffect == 1 ? " active" : "") + "\" onclick=\"E(1)\">Solid</button>");
  server.sendContent("<button class=\"btn" + String(currentEffect == 2 ? " active" : "") + "\" onclick=\"E(2)\">Ripple</button>");
  server.sendContent("<button class=\"btn" + String(currentEffect == 3 ? " active" : "") + "\" onclick=\"E(3)\">Rainbow</button>");
  server.sendContent("<button class=\"btn" + String(currentEffect == 4 ? " active" : "") + "\" onclick=\"E(4)\">Rain</button>");
  server.sendContent("<button class=\"btn" + String(currentEffect == 5 ? " active" : "") + "\" onclick=\"E(5)\">Matrix</button>");
  server.sendContent("<button class=\"btn" + String(currentEffect == 6 ? " active" : "") + "\" onclick=\"E(6)\">Fire</button>");
  server.sendContent("<button class=\"btn" + String(currentEffect == 7 ? " active" : "") + "\" onclick=\"E(7)\">Plasma</button>");
  server.sendContent("<button class=\"btn" + String(currentEffect == 8 ? " active" : "") + "\" onclick=\"E(8)\">Ocean</button>");
  server.sendContent("<button class=\"btn" + String(currentEffect == 9 ? " active" : "") + "\" onclick=\"E(9)\">Nebula</button>");
  server.sendContent("<button class=\"btn" + String(currentEffect == 10 ? " active" : "") + "\" onclick=\"E(10)\">Life</button>");
  server.sendContent("<button class=\"btn" + String(currentEffect == 11 ? " active" : "") + "\" onclick=\"E(11)\">Pong</button>");
  server.sendContent("<button class=\"btn" + String(currentEffect == 12 ? " active" : "") + "\" onclick=\"E(12)\">Metaballs</button>");
  server.sendContent("<button class=\"btn" + String(currentEffect == 13 ? " active" : "") + "\" onclick=\"E(13)\">Interference</button>");
  server.sendContent("<button class=\"btn" + String(currentEffect == 14 ? " active" : "") + "\" onclick=\"E(14)\">Noise</button>");
  server.sendContent("<button class=\"btn" + String(currentEffect == 15 ? " active" : "") + "\" onclick=\"E(15)\">Pool</button>");
  server.sendContent("<button class=\"btn" + String(currentEffect == 16 ? " active" : "") + "\" onclick=\"E(16)\">Rings</button>");
  server.sendContent("<button class=\"btn" + String(currentEffect == 17 ? " active" : "") + "\" onclick=\"E(17)\">Ball</button>");
  server.sendContent("</div>");

  // Settings sliders within effects card
  server.sendContent("<div class=\"slider-row\"><div class=\"slider-label\"><span>Brightness</span>");
  server.sendContent("<span class=\"slider-val\" id=\"bv\">" + String(currentBrightness) + "/50</span></div>");
  server.sendContent("<input type=\"range\" min=\"5\" max=\"50\" value=\"" + String(currentBrightness) + "\" oninput=\"B(this.value)\"></div>");

  server.sendContent("<div class=\"slider-row\"><div class=\"slider-label\"><span>Speed</span>");
  server.sendContent("<span class=\"slider-val\" id=\"sv\">" + String(effectSpeed) + "%</span></div>");
  server.sendContent("<input type=\"range\" min=\"10\" max=\"100\" value=\"" + String(effectSpeed) + "\" oninput=\"S(this.value)\"></div>");

  server.sendContent("<div class=\"rot-row\"><span>Rotation</span>");
  server.sendContent("<button class=\"rot-btn" + String(currentRotation == 0 ? " active" : "") + "\" onclick=\"R(0)\">0°</button>");
  server.sendContent("<button class=\"rot-btn" + String(currentRotation == 1 ? " active" : "") + "\" onclick=\"R(1)\">90°</button>");
  server.sendContent("<button class=\"rot-btn" + String(currentRotation == 2 ? " active" : "") + "\" onclick=\"R(2)\">180°</button>");
  server.sendContent("<button class=\"rot-btn" + String(currentRotation == 3 ? " active" : "") + "\" onclick=\"R(3)\">270°</button>");
  server.sendContent("</div></div></div>");

  // Statistics
  server.sendContent("<div class=\"card\"><div class=\"card-title\">Statistics</div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Uptime</span><span class=\"stat-val\" id=\"up\">" + formatUptime(uptime) + "</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Checks</span><span class=\"stat-val\" id=\"chk\">" + String(totalChecks) + "</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Success Rate</span><span class=\"stat-val\" id=\"rate\">" + String(successRate, 1) + "%</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Failed</span><span class=\"stat-val\" id=\"fail\">" + String(failedChecks) + "</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Last Outage</span><span class=\"stat-val\" id=\"last\">" + (lastDowntime > 0 ? formatUptime(lastDowntime) : "None") + "</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Total Downtime</span><span class=\"stat-val\" id=\"down\">" + formatUptime(totalDowntimeMs) + "</span></div>");
  server.sendContent("</div>");

  // Network
  server.sendContent("<div class=\"card\"><div class=\"card-title\">Network</div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">SSID</span><span class=\"stat-val\">" + storedSSID + "</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">IP</span><span class=\"stat-val\">" + WiFi.localIP().toString() + "</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Signal</span><span class=\"stat-val\" id=\"rssi\">" + String(WiFi.RSSI()) + " dBm</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">MAC</span><span class=\"stat-val\">" + WiFi.macAddress() + "</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Channel</span><span class=\"stat-val\">" + String(WiFi.channel()) + "</span></div>");
  server.sendContent("</div>");

  // System - collapsible, default collapsed
  server.sendContent("<div class=\"card\"><div class=\"card-title collapsible collapsed\" id=\"sysT\" onclick=\"T('sys')\"><span>System</span><span class=\"toggle\">▼</span></div>");
  server.sendContent("<div class=\"card-body collapsed\" id=\"sysB\">");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Architecture</span><span class=\"stat-val\">Dual Core ESP32-S3</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">CPU Freq</span><span class=\"stat-val\">" + String(ESP.getCpuFreqMHz()) + " MHz</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Free Heap</span><span class=\"stat-val\" id=\"heap\">" + String(ESP.getFreeHeap() / 1024) + " KB</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Min Free Heap</span><span class=\"stat-val\" id=\"minheap\">" + String(ESP.getMinFreeHeap() / 1024) + " KB</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Flash Size</span><span class=\"stat-val\">" + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Sketch Size</span><span class=\"stat-val\">" + String(ESP.getSketchSize() / 1024) + " KB</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Chip Temp</span><span class=\"stat-val\" id=\"temp\">" + String(getChipTemp(), 1) + "°C</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">OTA Updates</span><span class=\"stat-val good\">Enabled</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Firmware</span><span class=\"stat-val\">v" + String(FW_VERSION) + "</span></div>");
  server.sendContent("</div></div>");

  // Diagnostics - collapsible, default collapsed
  server.sendContent("<div class=\"card\"><div class=\"card-title collapsible collapsed\" id=\"diagT\" onclick=\"T('diag')\"><span>Diagnostics</span><span class=\"toggle\">▼</span></div>");
  server.sendContent("<div class=\"card-body collapsed\" id=\"diagB\">");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">LED FPS</span><span class=\"stat-val\" id=\"fps\">" + String(ledActualFPS, 1) + "</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Frame Time</span><span class=\"stat-val\" id=\"frameus\">" + String(ledFrameTimeUs) + " µs</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Max Frame Time</span><span class=\"stat-val\" id=\"maxframeus\">" + String(ledMaxFrameTimeUs) + " µs</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">LED Stack Free</span><span class=\"stat-val\" id=\"ledstack\">" + String(ledStackHighWater * 4) + " bytes</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Net Stack Free</span><span class=\"stat-val\" id=\"netstack\">" + String(netStackHighWater * 4) + " bytes</span></div>");
  server.sendContent("</div></div>");

  // Factory Reset - at bottom
  server.sendContent("<div class=\"card\"><div class=\"card-title\">Danger Zone</div>");
  server.sendContent("<button class=\"btn btn-danger\" style=\"width:100%\" onclick=\"factoryReset()\">Factory Reset</button>");
  server.sendContent("<p style=\"font-size:.65rem;color:#707088;margin-top:8px;text-align:center\">Clears WiFi, password, and all settings</p>");
  server.sendContent("</div></div>");

  // JavaScript
  server.sendContent("<script>");
  server.sendContent(FPSTR(DASHBOARD_JS));
  server.sendContent("</script></body></html>");
}

void handleEffect() {
  if (!checkAuth()) { sendUnauthorized(); return; }
  if (server.hasArg("e")) {
    int effect = server.arg("e").toInt();
    if (effect >= 0 && effect < NUM_EFFECTS) {
      currentEffect = effect;
      resetAllEffectState();  // Reset all effect state for clean start
      
      // Apply per-effect default brightness and speed
      currentBrightness = effectDefaults[effect][0];
      effectSpeed = effectDefaults[effect][1];
      pixels.setBrightness(currentBrightness);
      
      markSettingsChanged();
      Serial.print("Effect: ");
      Serial.print(effectNames[currentEffect]);
      Serial.print(" (brightness=");
      Serial.print(currentBrightness);
      Serial.print(", speed=");
      Serial.print(effectSpeed);
      Serial.println(")");
      
      // Return new values so UI can update
      String json = "{\"brightness\":" + String(currentBrightness) + ",\"speed\":" + String(effectSpeed) + "}";
      server.send(200, "application/json", json);
      return;
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleBrightness() {
  if (!checkAuth()) { sendUnauthorized(); return; }
  if (server.hasArg("b")) {
    int brightness = server.arg("b").toInt();
    if (brightness >= 5 && brightness <= 50) {
      currentBrightness = brightness;
      pixels.setBrightness(currentBrightness);
      markSettingsChanged();
      Serial.print("Brightness: ");
      Serial.println(currentBrightness);
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleRotation() {
  if (!checkAuth()) { sendUnauthorized(); return; }
  if (server.hasArg("r")) {
    int rotation = server.arg("r").toInt();
    if (rotation >= 0 && rotation <= 3) {
      currentRotation = rotation;
      markSettingsChanged();
      Serial.print("Rotation: ");
      Serial.println(rotation * 90);
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleSpeed() {
  if (!checkAuth()) { sendUnauthorized(); return; }
  if (server.hasArg("s")) {
    int speed = server.arg("s").toInt();
    if (speed >= 10 && speed <= 100) {
      effectSpeed = speed;
      markSettingsChanged();
      Serial.print("Speed: ");
      Serial.println(effectSpeed);
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleFactoryReset() {
  if (!checkAuth()) { sendUnauthorized(); return; }

  Serial.println("FACTORY RESET requested via web UI");

  // Clear ALL NVS data
  preferences.begin(NVS_NAMESPACE, false);
  preferences.clear();
  preferences.end();

  String json = "{\"success\":true,\"ssid\":\"" + String(CONFIG_AP_SSID) + "\"}";
  server.send(200, "application/json", json);

  // Flash LEDs to confirm
  for (int j = 0; j < 3; j++) {
    for (int i = 0; i < NUM_LEDS; i++) {
      pixels.setPixelColor(i, pixels.Color(80, 80, 80));
    }
    pixels.show();
    delay(150);
    pixels.clear();
    pixels.show();
    delay(150);
  }

  Serial.println("Rebooting...");
  delay(500);
  ESP.restart();
}

void handleStats() {
  if (!checkAuth()) { sendUnauthorized(); return; }

  float successRate = totalChecks > 0 ? (100.0 * successfulChecks / totalChecks) : 100;

  const char* stateStr;
  switch (currentState) {
    case STATE_INTERNET_OK: stateStr = "ONLINE"; break;
    case STATE_INTERNET_DEGRADED: stateStr = "DEGRADED"; break;
    case STATE_INTERNET_DOWN: stateStr = "OFFLINE"; break;
    case STATE_WIFI_LOST: stateStr = "NO WIFI"; break;
    default: stateStr = "STARTING"; break;
  }

  char json[768];
  snprintf(json, sizeof(json),
    "{\"state\":%d,\"stateText\":\"%s\",\"uptime\":%lu,\"checks\":%lu,"
    "\"rate\":%.1f,\"failed\":%lu,\"downtime\":%lu,\"lastOutage\":%lu,"
    "\"rssi\":%d,\"heap\":%u,\"minHeap\":%u,\"temp\":%.1f,\"cpuFreq\":%u,"
    "\"ledFps\":%.1f,\"ledFrameUs\":%lu,\"ledMaxFrameUs\":%lu,"
    "\"ledStack\":%lu,\"netStack\":%lu,"
    "\"effects\":19,\"dualCore\":true,\"version\":\"%s\"}",
    currentState, stateStr, millis() - bootTime, totalChecks,
    successRate, failedChecks, totalDowntimeMs, lastDowntime,
    WiFi.RSSI(), ESP.getFreeHeap(), ESP.getMinFreeHeap(), getChipTemp(),
    ESP.getCpuFreqMHz(),
    ledActualFPS, ledFrameTimeUs, ledMaxFrameTimeUs,
    ledStackHighWater * 4, netStackHighWater * 4,
    FW_VERSION);

  server.send(200, "application/json", json);
}

void setupWebServer() {
  // Collect headers for cookie-based auth
  const char* headerKeys[] = {"Cookie", "Authorization"};
  server.collectHeaders(headerKeys, 2);

  server.on("/", handleRoot);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/logout", handleLogout);
  server.on("/stats", handleStats);
  server.on("/effect", handleEffect);
  server.on("/brightness", handleBrightness);
  server.on("/rotation", handleRotation);
  server.on("/speed", handleSpeed);
  server.on("/factory-reset", handleFactoryReset);
  server.begin();
  Serial.println("Web server started");
}

// ===========================================
// OTA SETUP
// ===========================================

void setupOTA() {
  ArduinoOTA.setHostname("internet-monitor");
  // Use same password as web dashboard
  ArduinoOTA.setPassword(storedWebPassword.c_str());
  
  ArduinoOTA.onStart([]() {
    Serial.println("OTA starting...");
    // Pause LED task during OTA
    ledTaskPaused = true;
    vTaskDelay(pdMS_TO_TICKS(50));  // Wait for current frame
    // Disable watchdog during OTA to prevent resets
    esp_task_wdt_delete(NULL);
    fillMatrixImmediate(40, 0, 40);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("OTA complete!");
    fillMatrixImmediate(0, 40, 40);
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    int pct = progress / (total / 100);
    int ledsOn = (pct * NUM_LEDS) / 100;
    for (int i = 0; i < NUM_LEDS; i++) {
      pixels.setPixelColor(i, i < ledsOn ? pixels.Color(40, 0, 40) : pixels.Color(5, 0, 5));
    }
    pixels.show();
    yield();  // Let system tasks run
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]\n", error);
    fillMatrixImmediate(50, 0, 0);
    ledTaskPaused = false;  // Resume on error
    // Re-enable watchdog
    esp_task_wdt_add(NULL);
  });
  
  ArduinoOTA.begin();
  Serial.println("OTA ready");
}

// ===========================================
// CONFIG PORTAL
// ===========================================

String buildNetworkListHTML(int networkCount) {
  if (networkCount <= 0) {
    return "<div class='no-networks'>No networks found</div>";
  }

  // Pre-allocate to reduce fragmentation (~150 bytes per network)
  String html;
  html.reserve(networkCount * 150);

  char entry[200];  // Buffer for each network entry

  for (int i = 0; i < networkCount; i++) {
    String ssid = WiFi.SSID(i);

    // Skip hidden networks
    if (ssid.length() == 0) continue;

    int rssi = WiFi.RSSI(i);
    bool isOpen = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);

    // Signal strength bars
    const char* signal = (rssi >= -50) ? "||||" : (rssi >= -60) ? "|||" : (rssi >= -70) ? "||" : "|";

    // Escape SSID for JS/HTML (simple escape for common chars)
    String escaped = ssid;
    escaped.replace("\\", "\\\\");
    escaped.replace("'", "\\'");
    escaped.replace("\"", "&quot;");
    escaped.replace("<", "&lt;");
    escaped.replace(">", "&gt;");

    // Build entry with snprintf
    snprintf(entry, sizeof(entry),
      "<div class='network' onclick=\"sel('%s',%d)\"><span class='ssid'>%s</span>"
      "<span class='meta'><span class='sig'>%s</span>%s</span></div>",
      escaped.c_str(), isOpen ? 1 : 0, escaped.c_str(), signal,
      isOpen ? "" : "<span class='lock'>*</span>");

    html += entry;
    esp_task_wdt_reset();
  }

  return html;
}

void handlePortalRoot() {
  Serial.println("Portal request received: /");
  lastPortalActivity = millis();

  // Require auth for portal
  if (!checkAuth()) {
    server.send(200, "text/html", LOGIN_HTML);
    return;
  }

  // Chunked response for memory efficiency
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  server.sendContent("<!DOCTYPE html><html><head>");
  server.sendContent("<meta charset='UTF-8'>");
  server.sendContent("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  server.sendContent("<title>WiFi Setup</title><style>");
  server.sendContent(FPSTR(PORTAL_CSS));
  server.sendContent("</style></head><body><div class='wrap'>");

  // Header
  server.sendContent("<h1>Internet Monitor</h1>");
  server.sendContent("<p class='sub'>WIFI SETUP</p>");

  // Network list
  server.sendContent("<div class='card'><div class='card-title'>Select Network</div>");
  server.sendContent("<div id='networks'>");
  server.sendContent(cachedNetworkListHTML);
  server.sendContent("</div>");
  server.sendContent("<button class='btn scan' onclick='scan()'>Scan Again</button>");
  server.sendContent("</div>");

  // Password input (hidden initially)
  server.sendContent("<div class='card' id='pwcard' style='display:none'>");
  server.sendContent("<div class='card-title'>Enter Password</div>");
  server.sendContent("<p id='selssid'></p>");
  server.sendContent("<input type='password' id='pw' placeholder='WiFi Password'>");
  server.sendContent("</div>");

  // Admin password (always visible)
  server.sendContent("<div class='card'>");
  server.sendContent("<div class='card-title'>Dashboard Password</div>");
  server.sendContent("<input type='password' id='adminpw' placeholder='Admin password (default: admin)'>");
  server.sendContent("<p style='font-size:.7rem;color:#707088;margin-top:8px'>Leave blank to use default password: admin</p>");
  server.sendContent("<button class='btn connect' onclick='connect()'>Connect</button>");
  server.sendContent("</div>");

  // Status
  server.sendContent("<div class='status' id='status'></div>");

  server.sendContent("</div><script>");
  server.sendContent(FPSTR(PORTAL_JS));
  server.sendContent("</script></body></html>");
}

bool scanInProgress = false;

void handleScan() {
  if (!checkAuth()) { sendUnauthorized(); return; }
  lastPortalActivity = millis();

  int n = WiFi.scanComplete();

  if (n == WIFI_SCAN_RUNNING) {
    // Scan in progress
    server.send(200, "text/html", "<div class='no-networks'>Scanning...</div>");
    return;
  }

  if (n >= 0) {
    // Scan complete with results
    Serial.printf("Scan found %d networks\n", n);
    cachedNetworkListHTML = buildNetworkListHTML(n);
    WiFi.scanDelete();  // Free memory
    scanInProgress = false;
    server.send(200, "text/html", cachedNetworkListHTML);
    return;
  }

  // n == WIFI_SCAN_FAILED (-2): No scan running, start one
  if (!scanInProgress) {
    Serial.println("Starting async scan...");
    // Need AP_STA mode to scan while running AP
    if (WiFi.getMode() == WIFI_AP) {
      WiFi.mode(WIFI_AP_STA);
      delay(100);
    }
    WiFi.scanNetworks(true);  // true = async
    scanInProgress = true;
  }
  server.send(200, "text/html", "<div class='no-networks'>Scanning...</div>");
}

void handleConnect() {
  if (!checkAuth()) { sendUnauthorized(); return; }
  lastPortalActivity = millis();

  String ssid = server.arg("ssid");
  String password = server.arg("password");
  String adminPw = server.arg("adminpw");
  
  // Use default if blank
  if (adminPw.length() == 0) {
    adminPw = "admin";
  }

  Serial.print("Attempting connection to: ");
  Serial.println(ssid);

  // Save credentials and admin password to NVS first
  saveCredentialsToNVS(ssid, password);
  
  // Save admin password
  preferences.begin(NVS_NAMESPACE, false);
  preferences.putString(NVS_KEY_WEB_PASSWORD, adminPw);
  preferences.end();
  Serial.println("Admin password saved to NVS");

  // Properly disconnect and reset WiFi before reconnecting
  WiFi.disconnect(true);  // Disconnect and clear config
  delay(100);             // Give WiFi time to clean up
  
  // Try to connect
  WiFi.mode(WIFI_AP_STA);
  delay(100);
  WiFi.begin(ssid.c_str(), password.c_str());

  // Wait for connection (with timeout)
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(500);
    Serial.print(".");
    esp_task_wdt_reset();
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    String json = "{\"success\":true,\"ip\":\"" + WiFi.localIP().toString() + "\"}";
    server.send(200, "application/json", json);

    // Give time for response to be sent, then restart
    delay(2000);
    Serial.println("Restarting...");
    ESP.restart();
  } else {
    Serial.println("Connection failed");
    // Clear the bad credentials
    clearNVSCredentials();
    
    // Reset WiFi back to AP-only mode for portal
    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_AP);
    delay(100);
    scanInProgress = false;  // Reset scan state
    
    server.send(200, "application/json", "{\"success\":false,\"error\":\"Connection failed. Check password.\"}");
  }
}

void setupPortalWebServer() {
  // Collect headers for cookie-based auth
  const char* headerKeys[] = {"Cookie", "Authorization"};
  server.collectHeaders(headerKeys, 2);

  // Auth endpoints
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/logout", handleLogout);

  // Main portal page
  server.on("/", HTTP_GET, handlePortalRoot);
  server.on("/scan", HTTP_GET, handleScan);
  server.on("/connect", HTTP_POST, handleConnect);

  // Captive portal detection endpoints - redirect all to portal
  server.on("/generate_204", HTTP_GET, []() {
    lastPortalActivity = millis();
    server.sendHeader("Location", "http://192.168.4.1/");
    server.send(302);
  });
  server.on("/hotspot-detect.html", HTTP_GET, []() {
    lastPortalActivity = millis();
    server.sendHeader("Location", "http://192.168.4.1/");
    server.send(302);
  });
  server.on("/connecttest.txt", HTTP_GET, []() {
    lastPortalActivity = millis();
    server.sendHeader("Location", "http://192.168.4.1/");
    server.send(302);
  });
  server.on("/fwlink", HTTP_GET, []() {
    lastPortalActivity = millis();
    server.sendHeader("Location", "http://192.168.4.1/");
    server.send(302);
  });
  server.on("/ncsi.txt", HTTP_GET, []() {
    lastPortalActivity = millis();
    server.sendHeader("Location", "http://192.168.4.1/");
    server.send(302);
  });
  server.on("/redirect", HTTP_GET, []() {
    lastPortalActivity = millis();
    server.sendHeader("Location", "http://192.168.4.1/");
    server.send(302);
  });
  server.on("/canonical.html", HTTP_GET, []() {
    lastPortalActivity = millis();
    server.sendHeader("Location", "http://192.168.4.1/");
    server.send(302);
  });
  server.on("/success.txt", HTTP_GET, []() {
    lastPortalActivity = millis();
    server.sendHeader("Location", "http://192.168.4.1/");
    server.send(302);
  });

  // Catch-all for any other requests
  server.onNotFound([]() {
    Serial.print("Portal catch-all: ");
    Serial.println(server.uri());
    lastPortalActivity = millis();
    server.sendHeader("Location", "http://192.168.4.1/");
    server.send(302);
  });

  Serial.println("Portal web server routes configured");
}

void enterConfigMode() {
  Serial.println("\n=== ENTERING CONFIG MODE ===");

  // Reset watchdog before lengthy operations
  esp_task_wdt_reset();

  changeState(STATE_CONFIG_PORTAL);
  configPortalActive = true;
  lastPortalActivity = millis();

  // Stop the web server first (important if called at runtime)
  server.stop();
  server.close();
  delay(200);

  esp_task_wdt_reset();

  // Fully disconnect and reset WiFi
  WiFi.disconnect(true, true);  // disconnect and erase AP config
  WiFi.mode(WIFI_OFF);
  delay(500);

  esp_task_wdt_reset();

  // Scan networks in STA mode (better results)
  Serial.println("Scanning networks...");
  WiFi.mode(WIFI_STA);
  delay(200);

  int n = WiFi.scanNetworks(false, false, false, 300);  // Active scan, no hidden, 300ms per channel
  Serial.printf("Found %d networks\n", n);

  esp_task_wdt_reset();

  // Build network list HTML (with conservative memory handling)
  Serial.println("Building network list HTML...");
  cachedNetworkListHTML = "";  // Free any existing memory first
  cachedNetworkListHTML.reserve(n * 200);  // Pre-allocate to avoid reallocs
  cachedNetworkListHTML = buildNetworkListHTML(n);
  Serial.printf("Network list built (%d bytes)\n", cachedNetworkListHTML.length());

  // Free scan results memory
  WiFi.scanDelete();

  esp_task_wdt_reset();

  // Now switch to AP mode for portal
  WiFi.mode(WIFI_AP);
  delay(200);
  WiFi.softAP(CONFIG_AP_SSID, WEB_PASSWORD, CONFIG_AP_CHANNEL);  // Password protected
  delay(500);  // Give AP time to start

  Serial.print("AP SSID: ");
  Serial.println(CONFIG_AP_SSID);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // Start DNS server - redirect ALL domains to our IP
  dnsServer.start(53, "*", WiFi.softAPIP());

  // Setup portal web server
  setupPortalWebServer();
  server.begin();

  Serial.println("Config portal ready - connect to WiFi: " + String(CONFIG_AP_SSID));
}

// ===========================================
// WATCHDOG
// ===========================================

void setupWatchdog() {
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = WDT_TIMEOUT * 1000,
    .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
    .trigger_panic = true
  };
  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL);  // Add main task
  Serial.println("Watchdog enabled");
}

// ===========================================
// START LED TASK
// ===========================================

void startLEDTask() {
  xTaskCreatePinnedToCore(
    ledTask,              // Task function
    "LEDTask",            // Task name
    4096,                 // Stack size (bytes)
    NULL,                 // Task parameters
    LED_TASK_PRIORITY,    // Priority (higher = more important)
    &ledTaskHandle,       // Task handle
    LED_CORE              // Core to run on (0)
  );
  Serial.println("LED task created on Core 0");
}

// ===========================================
// START NETWORK TASK
// ===========================================

void startNetworkTask() {
  xTaskCreatePinnedToCore(
    networkTask,          // Task function
    "NetworkTask",        // Task name
    8192,                 // Stack size (bytes) - larger for HTTP
    NULL,                 // Task parameters
    NET_TASK_PRIORITY,    // Priority
    &networkTaskHandle,   // Task handle
    NETWORK_CORE          // Core to run on (1)
  );
  Serial.println("Network task created on Core 1");
}

// ===========================================
// SETUP
// ===========================================

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n\n=== Internet Monitor v" + String(FW_VERSION) + " ===");
  Serial.println("=== DUAL CORE ARCHITECTURE ===");
  Serial.printf("Main task running on Core %d\n", xPortGetCoreID());

  bootTime = millis();

  // Initialize lookup tables for fast math in effects
  initLookupTables();
  Serial.println("Sin/Cos lookup tables initialized");

  // Init LEDs
  pixels.begin();
  
  // Load settings from NVS (brightness, effect, etc.)
  loadSettingsFromNVS();

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
  
  // If web password wasn't set in NVS, use config.h fallback
  if (storedWebPassword == "admin" || storedWebPassword.length() == 0) {
    // Check if config.h has a non-default password
    if (strlen(WEB_PASSWORD) > 0 && strcmp(WEB_PASSWORD, "admin") != 0) {
      storedWebPassword = WEB_PASSWORD;
      Serial.println("Using config.h web password fallback");
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
  } else {
    Serial.println("\nWiFi connection failed - entering config mode");
    enterConfigMode();
    return;
  }

  Serial.println("Setup complete!\n");
  Serial.println("Architecture:");
  Serial.println("  Core 0: LED effects @ 60fps");
  Serial.println("  Core 1: Network monitoring");
  Serial.println("  Main:   Web server + OTA");
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
