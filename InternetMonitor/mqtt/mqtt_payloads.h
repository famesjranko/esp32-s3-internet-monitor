#ifndef MQTT_PAYLOADS_H
#define MQTT_PAYLOADS_H

/**
 * @file mqtt_payloads.h
 * @brief MQTT JSON payload builders using ArduinoJson
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "../config.h"
#include "../core/types.h"
#include "../core/state.h"
#include "mqtt_config.h"

// ===========================================
// EXTERNAL DECLARATIONS
// ===========================================

extern volatile int currentState;
extern SystemStats stats;
extern PerformanceMetrics perf;

// ===========================================
// STATE TEXT HELPERS
// ===========================================

/**
 * Get machine-readable state text
 */
inline const char* getStateText(int state) {
  switch (state) {
    case STATE_INTERNET_OK: return "online";
    case STATE_INTERNET_DEGRADED: return "degraded";
    case STATE_INTERNET_DOWN: return "offline";
    case STATE_WIFI_LOST: return "no_wifi";
    case STATE_CONFIG_PORTAL: return "setup";
    case STATE_CONNECTING_WIFI: return "connecting";
    case STATE_BOOTING: return "booting";
    default: return "unknown";
  }
}

/**
 * Get human-friendly state text
 */
inline const char* getStateFriendly(int state) {
  switch (state) {
    case STATE_INTERNET_OK: return "Online";
    case STATE_INTERNET_DEGRADED: return "Degraded";
    case STATE_INTERNET_DOWN: return "Offline";
    case STATE_WIFI_LOST: return "No WiFi";
    case STATE_CONFIG_PORTAL: return "Setup Mode";
    case STATE_CONNECTING_WIFI: return "Connecting";
    case STATE_BOOTING: return "Booting";
    default: return "Unknown";
  }
}

// ===========================================
// JSON PAYLOAD BUILDER
// ===========================================

/**
 * Build complete MQTT status payload
 * Uses ArduinoJson for clean serialization
 * @return JSON string with all status fields
 */
inline String buildMQTTPayload() {
  JsonDocument doc;
  
  float successRate = stats.totalChecks > 0 
    ? (100.0f * stats.successfulChecks / stats.totalChecks) 
    : 100.0f;
  
  unsigned long uptimeSeconds = (millis() - stats.bootTime) / 1000;
  
  // Status info
  doc["status"] = getStateText(currentState);
  doc["state"] = currentState;
  doc["state_text"] = getStateFriendly(currentState);
  
  // Uptime and checks
  doc["uptime_seconds"] = uptimeSeconds;
  doc["total_checks"] = stats.totalChecks;
  doc["successful_checks"] = stats.successfulChecks;
  doc["failed_checks"] = stats.failedChecks;
  doc["success_rate"] = serialized(String(successRate, 1));
  doc["consecutive_failures"] = stats.consecutiveFailures;
  
  // Downtime tracking
  doc["last_outage_seconds"] = stats.lastDowntime / 1000;
  doc["total_downtime_seconds"] = stats.totalDowntimeMs / 1000;
  
  // Network info
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["wifi_ssid"] = WiFi.SSID();
  doc["ip_address"] = WiFi.localIP().toString();
  
  // System info
  doc["free_heap"] = ESP.getFreeHeap();
  doc["temperature"] = serialized(String(temperatureRead(), 1));
  doc["led_fps"] = serialized(String(perf.ledActualFPS, 1));
  doc["firmware"] = FW_VERSION;
  
  String output;
  serializeJson(doc, output);
  return output;
}

// ===========================================
// SIMPLE STATUS VALUES (for individual topics)
// ===========================================

inline String buildStatusValue() {
  return String(getStateText(currentState));
}

inline String buildUptimeValue() {
  return String((millis() - stats.bootTime) / 1000);
}

inline String buildSuccessRateValue() {
  float rate = stats.totalChecks > 0 
    ? (100.0f * stats.successfulChecks / stats.totalChecks) 
    : 100.0f;
  return String(rate, 1);
}

inline String buildRSSIValue() {
  return String(WiFi.RSSI());
}

inline String buildTemperatureValue() {
  return String(temperatureRead(), 1);
}

inline String buildBinaryStatus() {
  return (currentState == STATE_INTERNET_OK) ? "1" : "0";
}

#endif // MQTT_PAYLOADS_H
