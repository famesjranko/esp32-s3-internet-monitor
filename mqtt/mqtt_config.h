#ifndef MQTT_CONFIG_H
#define MQTT_CONFIG_H

/**
 * @file mqtt_config.h
 * @brief MQTT configuration structure and NVS persistence
 * 
 * Defines MQTTConfig struct and functions for loading/saving
 * MQTT settings to non-volatile storage.
 */

#include <Arduino.h>
#include <Preferences.h>
#include "../config.h"

// ===========================================
// MQTT CONFIGURATION STRUCT
// ===========================================

struct MQTTConfig {
  bool enabled = false;
  
  // Broker connection
  char broker[64] = "";
  uint16_t port = MQTT_DEFAULT_PORT;
  char username[32] = "";
  char password[64] = "";
  char clientId[32] = "internet-monitor";
  
  // Topics - base topic, subtopics are appended
  char baseTopic[64] = MQTT_DEFAULT_TOPIC;
  
  // Publishing settings
  uint32_t publishIntervalMs = MQTT_DEFAULT_INTERVAL_MS;
  bool publishOnStateChange = true;
  
  // Features
  bool homeAssistantDiscovery = true;
  
  // Runtime state (not persisted)
  bool connected = false;
  unsigned long lastPublishTime = 0;
  unsigned long lastConnectAttempt = 0;
  int connectionFailures = 0;
  
  // Helper to check if configured
  bool isConfigured() const {
    return enabled && strlen(broker) > 0;
  }
  
  // Get full topic with suffix
  String getTopic(const char* suffix) const {
    String topic = baseTopic;
    if (suffix && strlen(suffix) > 0) {
      topic += "/";
      topic += suffix;
    }
    return topic;
  }
  
  // Get state topic
  String getStateTopic() const {
    return getTopic("state");
  }
  
  // Get availability topic
  String getAvailabilityTopic() const {
    return getTopic("availability");
  }
  
  // Get HA discovery topic for a sensor
  String getHADiscoveryTopic(const char* sensorType, const char* sensorId) const {
    // Format: homeassistant/{sensor_type}/internet_monitor/{sensor_id}/config
    String topic = "homeassistant/";
    topic += sensorType;
    topic += "/internet_monitor/";
    topic += sensorId;
    topic += "/config";
    return topic;
  }
};

// ===========================================
// GLOBAL MQTT CONFIG INSTANCE
// ===========================================

extern MQTTConfig mqttConfig;
extern Preferences preferences;

// ===========================================
// NVS PERSISTENCE FUNCTIONS
// ===========================================

inline void loadMQTTConfigFromNVS() {
  preferences.begin(NVS_NAMESPACE, true);  // read-only
  
  mqttConfig.enabled = preferences.getBool(NVS_KEY_MQTT_ENABLED, false);
  
  String broker = preferences.getString(NVS_KEY_MQTT_BROKER, "");
  strncpy(mqttConfig.broker, broker.c_str(), sizeof(mqttConfig.broker) - 1);
  
  mqttConfig.port = preferences.getUShort(NVS_KEY_MQTT_PORT, MQTT_DEFAULT_PORT);
  
  String user = preferences.getString(NVS_KEY_MQTT_USER, "");
  strncpy(mqttConfig.username, user.c_str(), sizeof(mqttConfig.username) - 1);
  
  String pass = preferences.getString(NVS_KEY_MQTT_PASS, "");
  strncpy(mqttConfig.password, pass.c_str(), sizeof(mqttConfig.password) - 1);
  
  String topic = preferences.getString(NVS_KEY_MQTT_TOPIC, MQTT_DEFAULT_TOPIC);
  strncpy(mqttConfig.baseTopic, topic.c_str(), sizeof(mqttConfig.baseTopic) - 1);
  
  mqttConfig.publishIntervalMs = preferences.getULong(NVS_KEY_MQTT_INTERVAL, MQTT_DEFAULT_INTERVAL_MS);
  mqttConfig.homeAssistantDiscovery = preferences.getBool(NVS_KEY_MQTT_HA_DISC, true);
  
  preferences.end();
  
  Serial.print("[MQTT] Config loaded - Enabled: ");
  Serial.print(mqttConfig.enabled ? "yes" : "no");
  if (mqttConfig.enabled) {
    Serial.print(", Broker: ");
    Serial.print(mqttConfig.broker);
    Serial.print(":");
    Serial.print(mqttConfig.port);
  }
  Serial.println();
}

inline void saveMQTTConfigToNVS() {
  preferences.begin(NVS_NAMESPACE, false);  // read-write
  
  preferences.putBool(NVS_KEY_MQTT_ENABLED, mqttConfig.enabled);
  preferences.putString(NVS_KEY_MQTT_BROKER, mqttConfig.broker);
  preferences.putUShort(NVS_KEY_MQTT_PORT, mqttConfig.port);
  preferences.putString(NVS_KEY_MQTT_USER, mqttConfig.username);
  preferences.putString(NVS_KEY_MQTT_PASS, mqttConfig.password);
  preferences.putString(NVS_KEY_MQTT_TOPIC, mqttConfig.baseTopic);
  preferences.putULong(NVS_KEY_MQTT_INTERVAL, mqttConfig.publishIntervalMs);
  preferences.putBool(NVS_KEY_MQTT_HA_DISC, mqttConfig.homeAssistantDiscovery);
  
  preferences.end();
  
  Serial.println("[MQTT] Config saved to NVS");
}

inline String getMQTTStatusText() {
  if (!mqttConfig.enabled) {
    return "Disabled";
  }
  if (strlen(mqttConfig.broker) == 0) {
    return "Not Configured";
  }
  if (mqttConfig.connected) {
    return "Connected";
  }
  if (mqttConfig.connectionFailures > 0) {
    return "Disconnected (" + String(mqttConfig.connectionFailures) + " failures)";
  }
  return "Connecting...";
}

#endif // MQTT_CONFIG_H
