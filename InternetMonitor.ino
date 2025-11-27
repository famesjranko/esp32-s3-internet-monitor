/*
 * ESP32-S3 Internet Monitor
 * 
 * Features:
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
#include "ui_login.h"
#include "ui_dashboard.h"
#include "ui_portal.h"

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
  EFFECT_PULSE,
  EFFECT_RAIN
};

// ===========================================
// GLOBALS
// ===========================================

Adafruit_NeoPixel pixels(NUM_LEDS, RGB_PIN, NEO_RGB + NEO_KHZ800);
WebServer server(80);

// State
int currentState = STATE_BOOTING;
int currentEffect = EFFECT_RAIN;
const char* effectNames[] = {"Off", "Solid", "Ripple", "Rainbow", "Pulse", "Rain"};
uint8_t currentBrightness = 21;
uint8_t currentRotation = 2;
uint8_t effectSpeed = 80;

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

// Colors for fading
uint8_t currentR = 0, currentG = 0, currentB = 0;
uint8_t targetR = 0, targetG = 0, targetB = 0;
unsigned long fadeStartTime = 0;
uint8_t fadeStartR = 0, fadeStartG = 0, fadeStartB = 0;

// Status flag for effects
bool isInternetOK = false;

// Config portal
DNSServer dnsServer;
Preferences preferences;
bool configPortalActive = false;
unsigned long lastPortalActivity = 0;
String cachedNetworkListHTML = "";


// Stored credentials (loaded from NVS or config.h fallback)
String storedSSID = "";
String storedPassword = "";

// NVS settings debounce
bool settingsPendingSave = false;
unsigned long lastSettingChangeTime = 0;


// Include effects after globals are defined
#include "effects.h"

// ===========================================
// INTERNET CHECK
// ===========================================

bool checkSingleUrl(const char* url) {
  HTTPClient http;
  http.setConnectTimeout(2000);
  http.setTimeout(3000);
  
  if (!http.begin(url)) {
    return false;
  }
  
  int code = http.GET();
  http.end();
  return (code == 204 || code == 200);
}

int checkInternet() {
  int successes = 0;
  
  for (int i = 0; i < 2; i++) {
    esp_task_wdt_reset();
    
    if (checkSingleUrl(checkUrls[i])) {
      successes++;
      if (successes >= 1) {
        esp_task_wdt_reset();
        return successes;
      }
    }
    
    esp_task_wdt_reset();
  }
  
  return successes;
}

// ===========================================
// STATE MANAGEMENT
// ===========================================

void changeState(State newState) {
  if (currentState == newState) return;
  
  Serial.print("State: ");
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

  switch (newState) {
    case STATE_BOOTING:
      setTargetColor(0, 0, 80);
      break;
    case STATE_CONNECTING_WIFI:
      setTargetColor(0, 40, 80);
      break;
    case STATE_CONFIG_PORTAL:
      setTargetColor(40, 0, 80);  // Purple
      break;
    case STATE_WIFI_LOST:
      setTargetColor(100, 0, 0);
      break;
    case STATE_INTERNET_OK:
      setTargetColor(0, 80, 0);
      break;
    case STATE_INTERNET_DEGRADED:
      setTargetColor(80, 60, 0);
      break;
    case STATE_INTERNET_DOWN:
      setTargetColor(100, 20, 0);
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

  // Only load if values exist (getBool/getUChar return default if key doesn't exist)
  if (preferences.isKey(NVS_KEY_BRIGHTNESS)) {
    currentBrightness = preferences.getUChar(NVS_KEY_BRIGHTNESS, currentBrightness);
  }
  if (preferences.isKey(NVS_KEY_EFFECT)) {
    currentEffect = preferences.getUChar(NVS_KEY_EFFECT, currentEffect);
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

  if (password == WEB_PASSWORD) {
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
  server.sendContent("<p class=\"sub\">ESP32-S3 MATRIX</p>");
  server.sendContent("</div><button class=\"logout\" onclick=\"logout()\">Logout</button></div>");

  // Status
  server.sendContent("<div class=\"status\">");
  server.sendContent("<span class=\"status-dot\" id=\"dot\" style=\"background:" + stateColor + ";box-shadow:0 0 8px " + stateColor + "\"></span>");
  server.sendContent("<span class=\"status-text\" id=\"stxt\" style=\"color:" + stateColor + "\">" + stateStr + "</span>");
  server.sendContent("</div>");

  // Effect card
  server.sendContent("<div class=\"card\"><div class=\"card-title\">Effect</div><div class=\"grid\">");
  server.sendContent("<button class=\"btn off" + String(currentEffect == 0 ? " active" : "") + "\" onclick=\"E(0)\">Off</button>");
  server.sendContent("<button class=\"btn" + String(currentEffect == 1 ? " active" : "") + "\" onclick=\"E(1)\">Solid</button>");
  server.sendContent("<button class=\"btn" + String(currentEffect == 2 ? " active" : "") + "\" onclick=\"E(2)\">Ripple</button>");
  server.sendContent("<button class=\"btn" + String(currentEffect == 3 ? " active" : "") + "\" onclick=\"E(3)\">Rainbow</button>");
  server.sendContent("<button class=\"btn" + String(currentEffect == 4 ? " active" : "") + "\" onclick=\"E(4)\">Pulse</button>");
  server.sendContent("<button class=\"btn" + String(currentEffect == 5 ? " active" : "") + "\" onclick=\"E(5)\">Rain</button>");
  server.sendContent("</div>");

  // Brightness
  server.sendContent("<div class=\"slider-row\"><div class=\"slider-label\"><span>Brightness</span>");
  server.sendContent("<span class=\"slider-val\" id=\"bv\">" + String(currentBrightness) + "/50</span></div>");
  server.sendContent("<input type=\"range\" min=\"5\" max=\"50\" value=\"" + String(currentBrightness) + "\" oninput=\"B(this.value)\"></div>");

  // Speed
  server.sendContent("<div class=\"slider-row\"><div class=\"slider-label\"><span>Speed</span>");
  server.sendContent("<span class=\"slider-val\" id=\"sv\">" + String(effectSpeed) + "%</span></div>");
  server.sendContent("<input type=\"range\" min=\"10\" max=\"100\" value=\"" + String(effectSpeed) + "\" oninput=\"S(this.value)\"></div>");

  // Rotation
  server.sendContent("<div class=\"rot-row\"><span>Rotation</span>");
  server.sendContent("<button class=\"rot-btn" + String(currentRotation == 0 ? " active" : "") + "\" onclick=\"R(0)\">0°</button>");
  server.sendContent("<button class=\"rot-btn" + String(currentRotation == 1 ? " active" : "") + "\" onclick=\"R(1)\">90°</button>");
  server.sendContent("<button class=\"rot-btn" + String(currentRotation == 2 ? " active" : "") + "\" onclick=\"R(2)\">180°</button>");
  server.sendContent("<button class=\"rot-btn" + String(currentRotation == 3 ? " active" : "") + "\" onclick=\"R(3)\">270°</button>");
  server.sendContent("</div></div>");

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
  server.sendContent("<button class=\"btn btn-danger\" style=\"width:100%;margin-top:12px\" onclick=\"resetWifi()\">Reset WiFi Settings</button>");
  server.sendContent("</div>");

  // System
  server.sendContent("<div class=\"card\"><div class=\"card-title\">System</div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Free Heap</span><span class=\"stat-val\" id=\"heap\">" + String(ESP.getFreeHeap() / 1024) + " KB</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Chip Temp</span><span class=\"stat-val\" id=\"temp\">" + String(getChipTemp(), 1) + "°C</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Firmware</span><span class=\"stat-val\">v" + String(FW_VERSION) + "</span></div>");
  server.sendContent("</div>");

  server.sendContent("<div class=\"footer\">OTA Updates Enabled</div></div>");

  // JavaScript
  server.sendContent("<script>");
  server.sendContent(FPSTR(DASHBOARD_JS));
  server.sendContent("</script></body></html>");
}

void handleEffect() {
  if (!checkAuth()) { sendUnauthorized(); return; }
  if (server.hasArg("e")) {
    int effect = server.arg("e").toInt();
    if (effect >= 0 && effect <= 5) {
      currentEffect = effect;
      markSettingsChanged();
      Serial.print("Effect: ");
      Serial.println(effectNames[currentEffect]);
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

void handleResetWifi() {
  if (!checkAuth()) { sendUnauthorized(); return; }

  Serial.println("WiFi reset requested - clearing credentials");

  // Clear WiFi credentials from NVS
  preferences.begin(NVS_NAMESPACE, false);
  preferences.remove(NVS_KEY_SSID);
  preferences.remove(NVS_KEY_PASSWORD);
  preferences.putBool(NVS_KEY_CONFIGURED, false);
  preferences.end();

  String json = "{\"success\":true,\"ssid\":\"" + String(CONFIG_AP_SSID) + "\"}";
  server.send(200, "application/json", json);

  delay(1000);
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

  char json[256];
  snprintf(json, sizeof(json),
    "{\"state\":%d,\"stateText\":\"%s\",\"uptime\":%lu,\"checks\":%lu,"
    "\"rate\":%.1f,\"failed\":%lu,\"downtime\":%lu,\"lastOutage\":%lu,"
    "\"rssi\":%d,\"heap\":%u,\"temp\":%.1f,\"version\":\"%s\"}",
    currentState, stateStr, millis() - bootTime, totalChecks,
    successRate, failedChecks, totalDowntimeMs, lastDowntime,
    WiFi.RSSI(), ESP.getFreeHeap(), getChipTemp(), FW_VERSION);

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
  server.on("/reset-wifi", handleResetWifi);
  server.begin();
  Serial.println("Web server started");
}

// ===========================================
// OTA SETUP
// ===========================================

void setupOTA() {
  ArduinoOTA.setHostname("internet-monitor");
  
  ArduinoOTA.onStart([]() {
    Serial.println("OTA starting...");
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
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]\n", error);
    fillMatrixImmediate(50, 0, 0);
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

  Serial.print("Attempting connection to: ");
  Serial.println(ssid);

  // Save credentials to NVS first
  saveCredentialsToNVS(ssid, password);

  // Try to connect
  WiFi.mode(WIFI_AP_STA);
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
  esp_task_wdt_add(NULL);
  Serial.println("Watchdog enabled");
}

// ===========================================
// SETUP
// ===========================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=== Internet Monitor v" + String(FW_VERSION) + " ===");

  bootTime = millis();

  // Load settings from NVS first (brightness, effect, etc.)
  loadSettingsFromNVS();

  // Init LEDs with loaded brightness
  pixels.begin();
  pixels.setBrightness(currentBrightness);
  fillMatrixImmediate(0, 0, 40);  // Blue during boot

  changeState(STATE_BOOTING);

  // Setup watchdog
  setupWatchdog();

  // Try to load credentials from NVS
  if (!loadCredentialsFromNVS()) {
    // Fall back to config.h
    Serial.println("No NVS credentials, checking config.h fallback");
    storedSSID = WIFI_SSID;
    storedPassword = WIFI_PASSWORD;
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
    updateFade();
    applyEffect();
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    setupWebServer();
    setupOTA();
    changeState(STATE_INTERNET_OK);
  } else {
    Serial.println("\nWiFi connection failed - entering config mode");
    enterConfigMode();
    return;
  }

  Serial.println("Setup complete!\n");
}

// ===========================================
// MAIN LOOP
// ===========================================

void loop() {
  esp_task_wdt_reset();

  // Handle config portal mode
  if (configPortalActive) {
    dnsServer.processNextRequest();
    server.handleClient();
    updateFade();
    applyEffect();

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

  ArduinoOTA.handle();
  server.handleClient();

  updateFade();
  applyEffect();

  // Check WiFi
  if (WiFi.status() != WL_CONNECTED) {
    if (currentState != STATE_WIFI_LOST) {
      Serial.println("WiFi lost!");
      changeState(STATE_WIFI_LOST);
    }
    esp_task_wdt_reset();
    delay(10);
    return;
  }

  esp_task_wdt_reset();

  if (currentState == STATE_WIFI_LOST) {
    Serial.println("WiFi recovered");
    changeState(STATE_INTERNET_OK);
  }

  // Internet check
  if (millis() - lastCheck >= CHECK_INTERVAL) {
    lastCheck = millis();
    totalChecks++;

    esp_task_wdt_reset();
    Serial.print("Checking... ");
    int successes = checkInternet();

    if (successes >= 1) {
      Serial.println("OK");
      successfulChecks++;
      consecutiveFailures = 0;
      consecutiveSuccesses++;

      if (currentState != STATE_INTERNET_OK) {
        changeState(STATE_INTERNET_OK);
      }
    } else {
      Serial.println("FAIL");
      failedChecks++;
      consecutiveFailures++;
      consecutiveSuccesses = 0;

      if (consecutiveFailures >= FAILURES_BEFORE_RED) {
        changeState(STATE_INTERNET_DOWN);
      } else {
        changeState(STATE_INTERNET_DEGRADED);
      }
    }
    esp_task_wdt_reset();
  }

  delay(10);
}
