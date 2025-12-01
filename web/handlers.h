#ifndef WEB_HANDLERS_H
#define WEB_HANDLERS_H

/**
 * @file handlers.h
 * @brief HTTP request handlers for the web dashboard
 * 
 * Implements all REST API endpoints for the dashboard including
 * effect control, brightness, rotation, speed, stats, and factory reset.
 * All endpoints (except login) require session authentication.
 */

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include "../config.h"
#include "../core/types.h"
#include "../core/state.h"
#include "../storage/nvs_manager.h"
#include "../mqtt/mqtt_config.h"
#include "auth.h"
#include "ui_login.h"
#include "ui_styles.h"
#include "ui_modal.h"
#include "ui_dashboard.h"

// ===========================================
// EXTERNAL DECLARATIONS
// ===========================================

extern WebServer server;
extern Adafruit_NeoPixel pixels;
extern SystemStats stats;
extern PerformanceMetrics perf;

extern volatile int currentState;
extern volatile int currentEffect;
extern volatile uint8_t currentBrightness;
extern volatile uint8_t currentRotation;
extern volatile uint8_t effectSpeed;

extern const char* effectNames[];
extern const uint8_t effectDefaults[][2];

extern String storedSSID;

// MQTT config
extern MQTTConfig mqttConfig;

// From effects.h
extern void resetAllEffectState();

// ===========================================
// DASHBOARD HANDLER
// ===========================================

inline void handleRoot() {
  if (!checkAuth()) {
    server.send(200, "text/html", LOGIN_HTML);
    return;
  }

  // Build dashboard using chunked response to reduce memory fragmentation
  unsigned long uptime = millis() - stats.bootTime;
  float successRate = stats.totalChecks > 0 ? (100.0 * stats.successfulChecks / stats.totalChecks) : 100;

  String stateStr, stateColor;
  switch (currentState) {
    case STATE_INTERNET_OK: stateStr = "ONLINE"; stateColor = "#22c55e"; break;
    case STATE_INTERNET_DEGRADED: stateStr = "DEGRADED"; stateColor = "#f59e0b"; break;
    case STATE_INTERNET_DOWN: stateStr = "OFFLINE"; stateColor = "#ef4444"; break;
    case STATE_WIFI_LOST: stateStr = "NO WIFI"; stateColor = "#ef4444"; break;
    case STATE_CONFIG_PORTAL: stateStr = "SETUP"; stateColor = "#c026d3"; break;
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
  server.sendContent(FPSTR(CSS_MODAL));
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
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Checks</span><span class=\"stat-val\" id=\"chk\">" + String(stats.totalChecks) + "</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Success Rate</span><span class=\"stat-val\" id=\"rate\">" + String(successRate, 1) + "%</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Failed</span><span class=\"stat-val\" id=\"fail\">" + String(stats.failedChecks) + "</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Last Outage</span><span class=\"stat-val\" id=\"last\">" + (stats.lastDowntime > 0 ? formatUptime(stats.lastDowntime) : "None") + "</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Total Downtime</span><span class=\"stat-val\" id=\"down\">" + formatUptime(stats.totalDowntimeMs) + "</span></div>");
  server.sendContent("</div>");

  // Network
  server.sendContent("<div class=\"card\"><div class=\"card-title\">Network</div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">SSID</span><span class=\"stat-val\">" + storedSSID + "</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">IP</span><span class=\"stat-val\">" + WiFi.localIP().toString() + "</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Signal</span><span class=\"stat-val\" id=\"rssi\">" + String(WiFi.RSSI()) + " dBm</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">MAC</span><span class=\"stat-val\">" + WiFi.macAddress() + "</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Channel</span><span class=\"stat-val\">" + String(WiFi.channel()) + "</span></div>");
  server.sendContent("</div>");

  // MQTT - collapsible, default collapsed
  String mqttStatusColor = mqttConfig.connected ? "#22c55e" : (mqttConfig.enabled ? "#f59e0b" : "#707088");
  server.sendContent("<div class=\"card\"><div class=\"card-title collapsible collapsed\" id=\"mqttT\" onclick=\"T('mqtt')\"><span>MQTT</span><span class=\"toggle\">▼</span></div>");
  server.sendContent("<div class=\"card-body collapsed\" id=\"mqttB\">");
  
  // Status row
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Status</span><span class=\"stat-val\" id=\"mqttStatus\" style=\"color:" + mqttStatusColor + "\">" + getMQTTStatusText() + "</span></div>");
  
  // Enable toggle
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Enabled</span>");
  server.sendContent("<label class=\"tog\" onclick=\"mqttToggle()\">");
  server.sendContent("<span class=\"tog-bg\" id=\"mqttEnBg\" style=\"background:" + String(mqttConfig.enabled ? "#4338ca" : "#303048") + "\"></span>");
  server.sendContent("<span class=\"tog-knob\" id=\"mqttEnKnob\" style=\"left:" + String(mqttConfig.enabled ? "22px" : "2px") + "\"></span>");
  server.sendContent("<input type=\"hidden\" id=\"mqttEn\" value=\"" + String(mqttConfig.enabled ? "1" : "0") + "\">");
  server.sendContent("</label></div>");
  
  // Broker
  server.sendContent("<div class=\"stat\" style=\"flex-wrap:wrap\"><span class=\"stat-label\">Broker</span>");
  server.sendContent("<input type=\"text\" id=\"mqttBroker\" value=\"" + String(mqttConfig.broker) + "\" placeholder=\"mqtt.example.com\" style=\"flex:1;min-width:120px;padding:6px 8px;background:#252540;border:1px solid #303048;border-radius:4px;color:#b8b8c8;font-size:.8rem\"></div>");
  
  // Port
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Port</span>");
  server.sendContent("<input type=\"number\" id=\"mqttPort\" value=\"" + String(mqttConfig.port) + "\" style=\"width:70px;padding:6px 8px;background:#252540;border:1px solid #303048;border-radius:4px;color:#b8b8c8;font-size:.8rem\"></div>");
  
  // Username
  server.sendContent("<div class=\"stat\" style=\"flex-wrap:wrap\"><span class=\"stat-label\">Username</span>");
  server.sendContent("<input type=\"text\" id=\"mqttUser\" value=\"" + String(mqttConfig.username) + "\" placeholder=\"(optional)\" style=\"flex:1;min-width:100px;padding:6px 8px;background:#252540;border:1px solid #303048;border-radius:4px;color:#b8b8c8;font-size:.8rem\"></div>");
  
  // Password
  server.sendContent("<div class=\"stat\" style=\"flex-wrap:wrap\"><span class=\"stat-label\">Password</span>");
  server.sendContent("<input type=\"password\" id=\"mqttPass\" placeholder=\"" + String(strlen(mqttConfig.password) > 0 ? "••••••••" : "(optional)") + "\" style=\"flex:1;min-width:100px;padding:6px 8px;background:#252540;border:1px solid #303048;border-radius:4px;color:#b8b8c8;font-size:.8rem\"></div>");
  
  // Topic
  server.sendContent("<div class=\"stat\" style=\"flex-wrap:wrap\"><span class=\"stat-label\">Base Topic</span>");
  server.sendContent("<input type=\"text\" id=\"mqttTopic\" value=\"" + String(mqttConfig.baseTopic) + "\" style=\"flex:1;min-width:120px;padding:6px 8px;background:#252540;border:1px solid #303048;border-radius:4px;color:#b8b8c8;font-size:.8rem\"></div>");
  
  // Interval
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Interval (sec)</span>");
  server.sendContent("<input type=\"number\" id=\"mqttInt\" value=\"" + String(mqttConfig.publishIntervalMs / 1000) + "\" min=\"5\" max=\"3600\" style=\"width:70px;padding:6px 8px;background:#252540;border:1px solid #303048;border-radius:4px;color:#b8b8c8;font-size:.8rem\"></div>");
  
  // HA Discovery toggle
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">HA Discovery</span>");
  server.sendContent("<label class=\"tog\" onclick=\"togHA()\">");
  server.sendContent("<span class=\"tog-bg\" id=\"mqttHABg\" style=\"background:" + String(mqttConfig.homeAssistantDiscovery ? "#4338ca" : "#303048") + "\"></span>");
  server.sendContent("<span class=\"tog-knob\" id=\"mqttHAKnob\" style=\"left:" + String(mqttConfig.homeAssistantDiscovery ? "22px" : "2px") + "\"></span>");
  server.sendContent("<input type=\"hidden\" id=\"mqttHA\" value=\"" + String(mqttConfig.homeAssistantDiscovery ? "1" : "0") + "\">");
  server.sendContent("</label></div>");
  
  // Buttons
  server.sendContent("<div style=\"display:flex;gap:8px;margin-top:12px\">");
  server.sendContent("<button class=\"btn\" style=\"flex:1\" onclick=\"mqttSave()\">Save</button>");
  server.sendContent("<button class=\"btn\" style=\"flex:1\" onclick=\"mqttTest()\">Test</button>");
  server.sendContent("</div>");
  
  server.sendContent("<p style=\"font-size:.6rem;color:#505068;margin-top:8px\">Publishes to: " + String(mqttConfig.baseTopic) + "/state</p>");
  server.sendContent("</div></div>");

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
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">LED FPS</span><span class=\"stat-val\" id=\"fps\">" + String(perf.ledActualFPS, 1) + "</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Frame Time</span><span class=\"stat-val\" id=\"frameus\">" + String(perf.ledFrameTimeUs) + " µs</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Max Frame Time</span><span class=\"stat-val\" id=\"maxframeus\">" + String(perf.ledMaxFrameTimeUs) + " µs</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">LED Stack Free</span><span class=\"stat-val\" id=\"ledstack\">" + String(perf.ledStackHighWater * 4) + " bytes</span></div>");
  server.sendContent("<div class=\"stat\"><span class=\"stat-label\">Net Stack Free</span><span class=\"stat-val\" id=\"netstack\">" + String(perf.netStackHighWater * 4) + " bytes</span></div>");
  server.sendContent("</div></div>");

  // Factory Reset - at bottom
  server.sendContent("<div class=\"card\"><div class=\"card-title\">Danger Zone</div>");
  server.sendContent("<button class=\"btn btn-danger\" style=\"width:100%\" onclick=\"factoryReset()\">Factory Reset</button>");
  server.sendContent("<p style=\"font-size:.65rem;color:#707088;margin-top:8px;text-align:center\">Clears WiFi, password, and all settings</p>");
  server.sendContent("</div></div>");

  // Modal overlay (for styled popups)
  server.sendContent(FPSTR(MODAL_HTML));

  // JavaScript
  server.sendContent("<script>");
  server.sendContent(FPSTR(MODAL_JS));
  server.sendContent(FPSTR(DASHBOARD_JS));
  server.sendContent("</script></body></html>");
}

// ===========================================
// API HANDLERS
// ===========================================

inline void handleEffect() {
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
      JsonDocument doc;
      doc["brightness"] = currentBrightness;
      doc["speed"] = effectSpeed;
      String output;
      serializeJson(doc, output);
      server.send(200, "application/json", output);
      return;
    }
  }
  sendSuccess("effect set");
}

inline void handleBrightness() {
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
  sendSuccess("brightness set");
}

inline void handleRotation() {
  if (!checkAuth()) { sendUnauthorized(); return; }
  if (server.hasArg("r")) {
    int rotation = server.arg("r").toInt();
    if (rotation >= ROTATION_0 && rotation <= ROTATION_270) {
      currentRotation = rotation;
      markSettingsChanged();
      Serial.print("Rotation: ");
      Serial.println(rotation * 90);
    }
  }
  sendSuccess("rotation set");
}

inline void handleSpeed() {
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
  sendSuccess("speed set");
}

inline void handleFactoryReset() {
  if (!checkAuth()) { sendUnauthorized(); return; }

  Serial.println("FACTORY RESET requested via web UI");

  // Clear ALL NVS data
  clearAllNVS();

  JsonDocument doc;
  doc["success"] = true;
  doc["ssid"] = CONFIG_AP_SSID;
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);

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

/**
 * Handle GET /stats
 * Returns comprehensive system statistics as JSON
 */
inline void handleStats() {
  if (!checkAuth()) { sendUnauthorized(); return; }

  JsonDocument doc;
  
  float successRate = stats.totalChecks > 0 
    ? (100.0 * stats.successfulChecks / stats.totalChecks) 
    : 100;

  const char* stateStr;
  switch (currentState) {
    case STATE_INTERNET_OK: stateStr = "ONLINE"; break;
    case STATE_INTERNET_DEGRADED: stateStr = "DEGRADED"; break;
    case STATE_INTERNET_DOWN: stateStr = "OFFLINE"; break;
    case STATE_WIFI_LOST: stateStr = "NO WIFI"; break;
    default: stateStr = "STARTING"; break;
  }

  // State info
  doc["state"] = currentState;
  doc["stateText"] = stateStr;
  
  // Uptime and checks
  doc["uptime"] = millis() - stats.bootTime;
  doc["checks"] = stats.totalChecks;
  doc["rate"] = serialized(String(successRate, 1));
  doc["failed"] = stats.failedChecks;
  doc["downtime"] = stats.totalDowntimeMs;
  doc["lastOutage"] = stats.lastDowntime;
  
  // Network info
  doc["rssi"] = WiFi.RSSI();
  
  // System info
  doc["heap"] = ESP.getFreeHeap();
  doc["minHeap"] = ESP.getMinFreeHeap();
  doc["temp"] = serialized(String(getChipTemp(), 1));
  doc["cpuFreq"] = ESP.getCpuFreqMHz();
  
  // Performance metrics
  doc["ledFps"] = serialized(String(perf.ledActualFPS, 1));
  doc["ledFrameUs"] = perf.ledFrameTimeUs;
  doc["ledMaxFrameUs"] = perf.ledMaxFrameTimeUs;
  doc["ledStack"] = perf.ledStackHighWater * 4;
  doc["netStack"] = perf.netStackHighWater * 4;
  
  // Static info
  doc["effects"] = NUM_EFFECTS;
  doc["dualCore"] = true;
  doc["version"] = FW_VERSION;

  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

#endif // WEB_HANDLERS_H
