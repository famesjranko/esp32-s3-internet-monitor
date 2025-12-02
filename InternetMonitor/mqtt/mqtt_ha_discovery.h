#ifndef MQTT_HA_DISCOVERY_H
#define MQTT_HA_DISCOVERY_H

/**
 * @file mqtt_ha_discovery.h
 * @brief Home Assistant MQTT auto-discovery message builders
 * 
 * Creates discovery payloads for 8 entities:
 * - Status (text sensor)
 * - Connectivity (binary sensor)
 * - Uptime, Success Rate, RSSI, Temperature, Failed Checks, Downtime (sensors)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "../config.h"
#include "mqtt_config.h"

// ===========================================
// UNIQUE ID HELPER
// ===========================================

/**
 * Build unique ID from MAC address suffix
 * @param suffix Entity identifier (e.g., "status", "uptime")
 * @return Unique ID like "imon_395180_status"
 */
inline String buildUniqueId(const char* suffix) {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  mac.toLowerCase();
  return "imon_" + mac.substring(6) + "_" + suffix;
}

/**
 * Get device MAC ID for grouping entities
 */
inline String getDeviceId() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  mac.toLowerCase();
  return "internet_monitor_" + mac;
}

// ===========================================
// DISCOVERY PAYLOAD BUILDER
// ===========================================

/**
 * Build a Home Assistant discovery message
 * Handles common fields and device info automatically
 * 
 * @param name Display name in HA
 * @param uniqueId Unique entity ID
 * @param valueTemplate Jinja template to extract value from state JSON
 * @param icon MDI icon name (optional)
 * @param unit Unit of measurement (optional)
 * @param deviceClass HA device class (optional)
 * @param entityCategory "diagnostic" or nullptr (optional)
 * @return JSON discovery payload
 */
inline String buildHADiscovery(
  const char* name,
  const char* uniqueId,
  const char* valueTemplate,
  const char* icon = nullptr,
  const char* unit = nullptr,
  const char* deviceClass = nullptr,
  const char* entityCategory = nullptr
) {
  JsonDocument doc;
  
  doc["name"] = name;
  doc["unique_id"] = uniqueId;
  doc["state_topic"] = mqttConfig.getStateTopic();
  doc["availability_topic"] = mqttConfig.getAvailabilityTopic();
  doc["value_template"] = valueTemplate;
  
  if (icon) doc["icon"] = icon;
  if (unit) doc["unit_of_measurement"] = unit;
  if (deviceClass) doc["device_class"] = deviceClass;
  if (entityCategory) doc["entity_category"] = entityCategory;
  
  // Device info block
  JsonObject device = doc["device"].to<JsonObject>();
  JsonArray identifiers = device["identifiers"].to<JsonArray>();
  identifiers.add(getDeviceId());
  device["name"] = "Internet Monitor";
  device["model"] = "ESP32-S3 Matrix";
  device["manufacturer"] = "DIY";
  device["sw_version"] = FW_VERSION;
  device["configuration_url"] = "http://" + WiFi.localIP().toString();
  
  String output;
  serializeJson(doc, output);
  return output;
}

/**
 * Build binary sensor discovery (different payload structure)
 */
inline String buildHADiscoveryBinary(
  const char* name,
  const char* uniqueId,
  const char* valueTemplate,
  const char* deviceClass = nullptr
) {
  JsonDocument doc;
  
  doc["name"] = name;
  doc["unique_id"] = uniqueId;
  doc["state_topic"] = mqttConfig.getStateTopic();
  doc["availability_topic"] = mqttConfig.getAvailabilityTopic();
  doc["value_template"] = valueTemplate;
  doc["payload_on"] = "ON";
  doc["payload_off"] = "OFF";
  
  if (deviceClass) doc["device_class"] = deviceClass;
  
  // Device info block
  JsonObject device = doc["device"].to<JsonObject>();
  JsonArray identifiers = device["identifiers"].to<JsonArray>();
  identifiers.add(getDeviceId());
  device["name"] = "Internet Monitor";
  device["model"] = "ESP32-S3 Matrix";
  device["manufacturer"] = "DIY";
  device["sw_version"] = FW_VERSION;
  device["configuration_url"] = "http://" + WiFi.localIP().toString();
  
  String output;
  serializeJson(doc, output);
  return output;
}

// ===========================================
// SENSOR DISCOVERY MESSAGES
// ===========================================

inline String buildHADiscoveryStatus() {
  return buildHADiscovery(
    "Status",
    buildUniqueId("status").c_str(),
    "{{ value_json.state_text }}",
    "mdi:web"
  );
}

inline String buildHADiscoveryConnectivity() {
  return buildHADiscoveryBinary(
    "Connectivity",
    buildUniqueId("connectivity").c_str(),
    "{{ 'ON' if value_json.status == 'online' else 'OFF' }}",
    "connectivity"
  );
}

inline String buildHADiscoveryUptime() {
  return buildHADiscovery(
    "Uptime",
    buildUniqueId("uptime").c_str(),
    "{{ value_json.uptime_seconds }}",
    "mdi:clock-outline",
    "s",
    "duration"
  );
}

inline String buildHADiscoverySuccessRate() {
  return buildHADiscovery(
    "Success Rate",
    buildUniqueId("success_rate").c_str(),
    "{{ value_json.success_rate }}",
    "mdi:percent",
    "%"
  );
}

inline String buildHADiscoveryRSSI() {
  return buildHADiscovery(
    "WiFi Signal",
    buildUniqueId("rssi").c_str(),
    "{{ value_json.wifi_rssi }}",
    nullptr,  // no icon, use device_class icon
    "dBm",
    "signal_strength",
    "diagnostic"
  );
}

inline String buildHADiscoveryTemperature() {
  return buildHADiscovery(
    "CPU Temperature",
    buildUniqueId("temperature").c_str(),
    "{{ value_json.temperature }}",
    nullptr,  // no icon, use device_class icon
    "°C",
    "temperature",
    "diagnostic"
  );
}

inline String buildHADiscoveryFailedChecks() {
  return buildHADiscovery(
    "Failed Checks",
    buildUniqueId("failed_checks").c_str(),
    "{{ value_json.failed_checks }}",
    "mdi:alert-circle-outline"
  );
}

inline String buildHADiscoveryDowntime() {
  return buildHADiscovery(
    "Total Downtime",
    buildUniqueId("downtime").c_str(),
    "{{ value_json.total_downtime_seconds }}",
    "mdi:timer-off-outline",
    "s",
    "duration"
  );
}

#endif // MQTT_HA_DISCOVERY_H
