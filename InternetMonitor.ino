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
#include <ArduinoOTA.h>
#include <Adafruit_NeoPixel.h>
#include <esp_task_wdt.h>

#include "config.h"
#include "ui_login.h"
#include "ui_dashboard.h"

// ===========================================
// STATE MACHINE
// ===========================================

enum State {
  STATE_BOOTING,
  STATE_CONNECTING_WIFI,
  STATE_WIFI_LOST,
  STATE_INTERNET_OK,
  STATE_INTERNET_DEGRADED,
  STATE_INTERNET_DOWN
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
unsigned long lastHeartbeat = 0;
unsigned long stateChangeTime = 0;
unsigned long bootTime = 0;

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
  
  switch (newState) {
    case STATE_BOOTING:
      setTargetColor(0, 0, 80);
      break;
    case STATE_CONNECTING_WIFI:
      setTargetColor(0, 40, 80);
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

bool checkAuth() {
  if (server.hasArg("key")) {
    return server.arg("key") == WEB_PASSWORD;
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
// WEB HANDLERS
// ===========================================

void handleRoot() {
  if (!checkAuth()) {
    server.send(200, "text/html", LOGIN_HTML);
    return;
  }

  // Build dashboard
  unsigned long uptime = millis() - bootTime;
  float successRate = totalChecks > 0 ? (100.0 * successfulChecks / totalChecks) : 100;
  String key = server.arg("key");
  
  String stateStr, stateColor;
  switch (currentState) {
    case STATE_INTERNET_OK: stateStr = "ONLINE"; stateColor = "#22c55e"; break;
    case STATE_INTERNET_DEGRADED: stateStr = "DEGRADED"; stateColor = "#f59e0b"; break;
    case STATE_INTERNET_DOWN: stateStr = "OFFLINE"; stateColor = "#ef4444"; break;
    case STATE_WIFI_LOST: stateStr = "NO WIFI"; stateColor = "#ef4444"; break;
    default: stateStr = "STARTING"; stateColor = "#3b82f6"; break;
  }

  // Build HTML
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset=\"UTF-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<title>Internet Monitor</title>";
  html += "<style>";
  html += FPSTR(DASHBOARD_CSS);
  html += "</style></head><body>";
  
  html += "<div class=\"wrap\">";
  
  // Header
  html += "<div class=\"hdr\"><div class=\"hdr-left\">";
  html += "<h1>Internet Monitor</h1>";
  html += "<p class=\"sub\">ESP32-S3 MATRIX</p>";
  html += "</div><button class=\"logout\" onclick=\"logout()\">Logout</button></div>";
  
  // Status
  html += "<div class=\"status\">";
  html += "<span class=\"status-dot\" id=\"dot\" style=\"background:" + stateColor + ";box-shadow:0 0 8px " + stateColor + "\"></span>";
  html += "<span class=\"status-text\" id=\"stxt\" style=\"color:" + stateColor + "\">" + stateStr + "</span>";
  html += "</div>";
  
  // Effect card
  html += "<div class=\"card\"><div class=\"card-title\">Effect</div>";
  html += "<div class=\"grid\">";
  html += "<button class=\"btn off" + String(currentEffect == 0 ? " active" : "") + "\" onclick=\"E(0)\">Off</button>";
  html += "<button class=\"btn" + String(currentEffect == 1 ? " active" : "") + "\" onclick=\"E(1)\">Solid</button>";
  html += "<button class=\"btn" + String(currentEffect == 2 ? " active" : "") + "\" onclick=\"E(2)\">Ripple</button>";
  html += "<button class=\"btn" + String(currentEffect == 3 ? " active" : "") + "\" onclick=\"E(3)\">Rainbow</button>";
  html += "<button class=\"btn" + String(currentEffect == 4 ? " active" : "") + "\" onclick=\"E(4)\">Pulse</button>";
  html += "<button class=\"btn" + String(currentEffect == 5 ? " active" : "") + "\" onclick=\"E(5)\">Rain</button>";
  html += "</div>";
  
  // Brightness
  html += "<div class=\"slider-row\"><div class=\"slider-label\"><span>Brightness</span>";
  html += "<span class=\"slider-val\" id=\"bv\">" + String(currentBrightness) + "/50</span></div>";
  html += "<input type=\"range\" min=\"5\" max=\"50\" value=\"" + String(currentBrightness) + "\" oninput=\"B(this.value)\"></div>";
  
  // Speed
  html += "<div class=\"slider-row\"><div class=\"slider-label\"><span>Speed</span>";
  html += "<span class=\"slider-val\" id=\"sv\">" + String(effectSpeed) + "%</span></div>";
  html += "<input type=\"range\" min=\"10\" max=\"100\" value=\"" + String(effectSpeed) + "\" oninput=\"S(this.value)\"></div>";
  
  // Rotation
  html += "<div class=\"rot-row\"><span>Rotation</span>";
  html += "<button class=\"rot-btn" + String(currentRotation == 0 ? " active" : "") + "\" onclick=\"R(0)\">0°</button>";
  html += "<button class=\"rot-btn" + String(currentRotation == 1 ? " active" : "") + "\" onclick=\"R(1)\">90°</button>";
  html += "<button class=\"rot-btn" + String(currentRotation == 2 ? " active" : "") + "\" onclick=\"R(2)\">180°</button>";
  html += "<button class=\"rot-btn" + String(currentRotation == 3 ? " active" : "") + "\" onclick=\"R(3)\">270°</button>";
  html += "</div></div>";
  
  // Statistics
  html += "<div class=\"card\"><div class=\"card-title\">Statistics</div>";
  html += "<div class=\"stat\"><span class=\"stat-label\">Uptime</span><span class=\"stat-val\" id=\"up\">" + formatUptime(uptime) + "</span></div>";
  html += "<div class=\"stat\"><span class=\"stat-label\">Checks</span><span class=\"stat-val\" id=\"chk\">" + String(totalChecks) + "</span></div>";
  html += "<div class=\"stat\"><span class=\"stat-label\">Success Rate</span><span class=\"stat-val\" id=\"rate\">" + String(successRate, 1) + "%</span></div>";
  html += "<div class=\"stat\"><span class=\"stat-label\">Failed</span><span class=\"stat-val\" id=\"fail\">" + String(failedChecks) + "</span></div>";
  html += "<div class=\"stat\"><span class=\"stat-label\">Last Outage</span><span class=\"stat-val\" id=\"last\">" + (lastDowntime > 0 ? formatUptime(lastDowntime) : "None") + "</span></div>";
  html += "<div class=\"stat\"><span class=\"stat-label\">Total Downtime</span><span class=\"stat-val\" id=\"down\">" + formatUptime(totalDowntimeMs) + "</span></div>";
  html += "</div>";
  
  // Network
  html += "<div class=\"card\"><div class=\"card-title\">Network</div>";
  html += "<div class=\"stat\"><span class=\"stat-label\">SSID</span><span class=\"stat-val\">" + String(WIFI_SSID) + "</span></div>";
  html += "<div class=\"stat\"><span class=\"stat-label\">IP</span><span class=\"stat-val\">" + WiFi.localIP().toString() + "</span></div>";
  html += "<div class=\"stat\"><span class=\"stat-label\">Signal</span><span class=\"stat-val\" id=\"rssi\">" + String(WiFi.RSSI()) + " dBm</span></div>";
  html += "<div class=\"stat\"><span class=\"stat-label\">MAC</span><span class=\"stat-val\">" + WiFi.macAddress() + "</span></div>";
  html += "<div class=\"stat\"><span class=\"stat-label\">Channel</span><span class=\"stat-val\">" + String(WiFi.channel()) + "</span></div>";
  html += "</div>";
  
  // System
  html += "<div class=\"card\"><div class=\"card-title\">System</div>";
  html += "<div class=\"stat\"><span class=\"stat-label\">Free Heap</span><span class=\"stat-val\" id=\"heap\">" + String(ESP.getFreeHeap() / 1024) + " KB</span></div>";
  html += "<div class=\"stat\"><span class=\"stat-label\">Chip Temp</span><span class=\"stat-val\" id=\"temp\">" + String(getChipTemp(), 1) + "°C</span></div>";
  html += "<div class=\"stat\"><span class=\"stat-label\">Firmware</span><span class=\"stat-val\">v" + String(FW_VERSION) + "</span></div>";
  html += "</div>";
  
  html += "<div class=\"footer\">OTA Updates Enabled</div>";
  html += "</div>";
  
  // JavaScript
  html += "<script>";
  String js = FPSTR(DASHBOARD_JS);
  js.replace("%KEY%", key);
  html += js;
  html += "</script></body></html>";

  server.send(200, "text/html", html);
}

void handleEffect() {
  if (!checkAuth()) { sendUnauthorized(); return; }
  if (server.hasArg("e")) {
    int effect = server.arg("e").toInt();
    if (effect >= 0 && effect <= 5) {
      currentEffect = effect;
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
      Serial.print("Speed: ");
      Serial.println(effectSpeed);
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleStats() {
  if (!checkAuth()) { sendUnauthorized(); return; }
  
  float successRate = totalChecks > 0 ? (100.0 * successfulChecks / totalChecks) : 100;
  
  String stateStr;
  switch (currentState) {
    case STATE_INTERNET_OK: stateStr = "ONLINE"; break;
    case STATE_INTERNET_DEGRADED: stateStr = "DEGRADED"; break;
    case STATE_INTERNET_DOWN: stateStr = "OFFLINE"; break;
    case STATE_WIFI_LOST: stateStr = "NO WIFI"; break;
    default: stateStr = "STARTING"; break;
  }
  
  String json = "{";
  json += "\"state\":" + String(currentState) + ",";
  json += "\"stateText\":\"" + stateStr + "\",";
  json += "\"uptime\":" + String(millis() - bootTime) + ",";
  json += "\"checks\":" + String(totalChecks) + ",";
  json += "\"rate\":" + String(successRate, 1) + ",";
  json += "\"failed\":" + String(failedChecks) + ",";
  json += "\"downtime\":" + String(totalDowntimeMs) + ",";
  json += "\"lastOutage\":" + String(lastDowntime) + ",";
  json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"temp\":" + String(getChipTemp(), 1) + ",";
  json += "\"version\":\"" + String(FW_VERSION) + "\"";
  json += "}";
  
  server.send(200, "application/json", json);
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/stats", handleStats);
  server.on("/effect", handleEffect);
  server.on("/brightness", handleBrightness);
  server.on("/rotation", handleRotation);
  server.on("/speed", handleSpeed);
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
  
  // Init LEDs
  pixels.begin();
  pixels.setBrightness(currentBrightness);
  fillMatrixImmediate(0, 0, 40);
  
  changeState(STATE_CONNECTING_WIFI);
  
  // Connect WiFi
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
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
    Serial.println("\nWiFi failed!");
    changeState(STATE_WIFI_LOST);
  }
  
  setupWatchdog();
  Serial.println("Setup complete!\n");
}

// ===========================================
// MAIN LOOP
// ===========================================

void loop() {
  esp_task_wdt_reset();
  
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
