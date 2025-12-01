#ifndef WEB_MQTT_HANDLERS_H
#define WEB_MQTT_HANDLERS_H

/**
 * @file mqtt_handlers.h
 * @brief Web API handlers for MQTT configuration
 */

#include <Arduino.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "../mqtt/mqtt_config.h"
#include "../mqtt/mqtt_manager.h"
#include "auth.h"

// ===========================================
// EXTERNAL DECLARATIONS
// ===========================================

extern WebServer server;
extern MQTTConfig mqttConfig;

// ===========================================
// MQTT CONFIG GET HANDLER
// ===========================================

/**
 * Handle GET /api/mqtt/config
 * Returns current MQTT configuration
 */
inline void handleMqttGetConfig() {
  if (!checkAuth()) { sendUnauthorized(); return; }
  
  JsonDocument doc;
  
  doc["enabled"] = mqttConfig.enabled;
  doc["broker"] = mqttConfig.broker;
  doc["port"] = mqttConfig.port;
  doc["username"] = mqttConfig.username;
  doc["topic"] = mqttConfig.baseTopic;
  doc["interval"] = mqttConfig.publishIntervalMs / 1000;  // Return as seconds
  doc["ha_discovery"] = mqttConfig.homeAssistantDiscovery;
  doc["connected"] = mqttConfig.connected;
  doc["status"] = getMQTTStatusText();
  
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

// ===========================================
// MQTT CONFIG SAVE HANDLER
// ===========================================

/**
 * Handle POST /api/mqtt/config
 * Saves MQTT configuration changes
 */
inline void handleMqttSaveConfig() {
  if (!checkAuth()) { sendUnauthorized(); return; }
  
  bool wasEnabled = mqttConfig.enabled;
  bool needReconnect = false;
  
  // Parse arguments
  if (server.hasArg("enabled")) {
    mqttConfig.enabled = (server.arg("enabled") == "true" || server.arg("enabled") == "1");
  }
  
  if (server.hasArg("broker")) {
    String broker = server.arg("broker");
    broker.trim();
    if (strcmp(mqttConfig.broker, broker.c_str()) != 0) {
      strncpy(mqttConfig.broker, broker.c_str(), sizeof(mqttConfig.broker) - 1);
      needReconnect = true;
    }
  }
  
  if (server.hasArg("port")) {
    uint16_t port = server.arg("port").toInt();
    if (port > 0 && port != mqttConfig.port) {
      mqttConfig.port = port;
      needReconnect = true;
    }
  }
  
  if (server.hasArg("username")) {
    String user = server.arg("username");
    user.trim();
    if (strcmp(mqttConfig.username, user.c_str()) != 0) {
      strncpy(mqttConfig.username, user.c_str(), sizeof(mqttConfig.username) - 1);
      needReconnect = true;
    }
  }
  
  if (server.hasArg("password")) {
    String pass = server.arg("password");
    // Only update if not empty (don't clear on empty submit)
    if (pass.length() > 0) {
      strncpy(mqttConfig.password, pass.c_str(), sizeof(mqttConfig.password) - 1);
      needReconnect = true;
    }
  }
  
  if (server.hasArg("topic")) {
    String topic = server.arg("topic");
    topic.trim();
    if (topic.length() > 0 && strcmp(mqttConfig.baseTopic, topic.c_str()) != 0) {
      strncpy(mqttConfig.baseTopic, topic.c_str(), sizeof(mqttConfig.baseTopic) - 1);
      mqttResetHADiscovery();  // Need to republish discovery with new topics
    }
  }
  
  if (server.hasArg("interval")) {
    uint32_t interval = server.arg("interval").toInt();
    if (interval >= 5) {  // Minimum 5 seconds
      mqttConfig.publishIntervalMs = interval * 1000;
    }
  }
  
  if (server.hasArg("ha_discovery")) {
    bool haDisc = (server.arg("ha_discovery") == "true" || server.arg("ha_discovery") == "1");
    if (haDisc != mqttConfig.homeAssistantDiscovery) {
      mqttConfig.homeAssistantDiscovery = haDisc;
      if (haDisc) {
        mqttResetHADiscovery();  // Republish discovery
      }
    }
  }
  
  // Save to NVS
  saveMQTTConfigToNVS();
  
  // Handle connection changes
  if (needReconnect && mqttConfig.enabled) {
    mqttDisconnect();
    // Connection will be re-established in mqttLoop()
  }
  
  if (!mqttConfig.enabled && wasEnabled) {
    mqttDisconnect();
  }
  
  // Return updated status
  JsonDocument doc;
  doc["success"] = true;
  doc["status"] = getMQTTStatusText();
  doc["connected"] = mqttConfig.connected;
  
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

// ===========================================
// MQTT STATUS HANDLER
// ===========================================

/**
 * Handle GET /api/mqtt/status
 * Returns current MQTT connection status
 */
inline void handleMqttStatus() {
  if (!checkAuth()) { sendUnauthorized(); return; }
  
  JsonDocument doc;
  
  doc["enabled"] = mqttConfig.enabled;
  doc["connected"] = mqttConfig.connected;
  doc["status"] = getMQTTStatusText();
  doc["failures"] = mqttConfig.connectionFailures;
  doc["last_publish"] = (millis() - mqttConfig.lastPublishTime) / 1000;
  
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

// ===========================================
// MQTT TEST CONNECTION HANDLER
// ===========================================

/**
 * Handle POST /api/mqtt/test
 * Tests MQTT connection with provided or saved credentials
 */
inline void handleMqttTest() {
  if (!checkAuth()) { sendUnauthorized(); return; }
  
  // Save current config temporarily
  char oldBroker[64];
  uint16_t oldPort = mqttConfig.port;
  char oldUser[32];
  char oldPass[64];
  bool tempConfig = false;
  
  if (server.hasArg("broker") && server.arg("broker").length() > 0) {
    tempConfig = true;
    strncpy(oldBroker, mqttConfig.broker, sizeof(oldBroker));
    strncpy(oldUser, mqttConfig.username, sizeof(oldUser));
    strncpy(oldPass, mqttConfig.password, sizeof(oldPass));
    
    strncpy(mqttConfig.broker, server.arg("broker").c_str(), sizeof(mqttConfig.broker) - 1);
    
    if (server.hasArg("port")) {
      mqttConfig.port = server.arg("port").toInt();
    }
    if (server.hasArg("username")) {
      strncpy(mqttConfig.username, server.arg("username").c_str(), sizeof(mqttConfig.username) - 1);
    }
    if (server.hasArg("password") && server.arg("password").length() > 0) {
      strncpy(mqttConfig.password, server.arg("password").c_str(), sizeof(mqttConfig.password) - 1);
    }
  }
  
  // Disconnect first
  mqttDisconnect();
  
  // Test connection (may block for up to 2 seconds)
  bool success = mqttTestConnection();
  
  // Restore config if using temp values
  if (tempConfig) {
    strncpy(mqttConfig.broker, oldBroker, sizeof(mqttConfig.broker));
    mqttConfig.port = oldPort;
    strncpy(mqttConfig.username, oldUser, sizeof(mqttConfig.username));
    strncpy(mqttConfig.password, oldPass, sizeof(mqttConfig.password));
  }
  
  JsonDocument doc;
  doc["success"] = success;
  doc["message"] = success ? "Connection successful" : "Connection failed";
  
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

#endif // WEB_MQTT_HANDLERS_H
