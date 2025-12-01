#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

/**
 * @file mqtt_manager.h
 * @brief MQTT client management with Home Assistant integration
 * 
 * Features:
 * - Runs in dedicated FreeRTOS task (won't block network checks)
 * - Automatic reconnection with rate limiting
 * - Home Assistant auto-discovery
 * - State publishing with configurable interval
 * - Last Will Testament for offline detection
 */

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "../config.h"
#include "mqtt_config.h"
#include "mqtt_payloads.h"
#include "mqtt_ha_discovery.h"

// ===========================================
// EXTERNAL DECLARATIONS
// ===========================================

extern MQTTConfig mqttConfig;
extern volatile int currentState;

// ===========================================
// MQTT CLIENT INSTANCES
// ===========================================

WiFiClient mqttWifiClient;
PubSubClient mqttClient(mqttWifiClient);

// Task handle for MQTT task
TaskHandle_t mqttTaskHandle = NULL;

// Track state changes for immediate publishing
static volatile int lastPublishedState = -1;
static volatile bool haDiscoveryPublished = false;

// ===========================================
// MQTT TASK SETTINGS
// ===========================================

#define MQTT_TASK_STACK_SIZE  4096
#define MQTT_TASK_PRIORITY    1       // Same as network task
#define MQTT_TASK_CORE        1       // Run on Core 1
#define MQTT_LOOP_DELAY_MS    100     // Check every 100ms
#define MQTT_SOCKET_TIMEOUT   2       // 2 second socket timeout (short!)

// ===========================================
// MQTT CALLBACKS
// ===========================================

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Handle incoming messages if needed in the future
  Serial.print("[MQTT] Message on topic: ");
  Serial.println(topic);
}

// ===========================================
// MQTT CONNECTION (called from MQTT task only)
// ===========================================

/**
 * Connect to MQTT broker
 * Rate-limited to prevent hammering broker on failures.
 * May block for up to MQTT_SOCKET_TIMEOUT seconds.
 * 
 * @return true if connected successfully
 */
inline bool mqttConnect() {
  if (!mqttConfig.isConfigured()) {
    return false;
  }
  
  if (mqttClient.connected()) {
    return true;
  }
  
  // Rate limit reconnection attempts
  unsigned long now = millis();
  if (now - mqttConfig.lastConnectAttempt < MQTT_RECONNECT_INTERVAL_MS) {
    return false;
  }
  mqttConfig.lastConnectAttempt = now;
  
  Serial.printf("[MQTT] Connecting to %s:%d... ", mqttConfig.broker, mqttConfig.port);
  
  // Configure client with SHORT timeout and larger buffer
  mqttClient.setServer(mqttConfig.broker, mqttConfig.port);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setKeepAlive(MQTT_KEEPALIVE_SEC);
  mqttClient.setSocketTimeout(MQTT_SOCKET_TIMEOUT);
  mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
  
  // Build LWT topic
  String availTopic = mqttConfig.getAvailabilityTopic();
  
  // Connect (may block for up to MQTT_SOCKET_TIMEOUT seconds)
  bool connected = false;
  if (strlen(mqttConfig.username) > 0) {
    connected = mqttClient.connect(
      mqttConfig.clientId,
      mqttConfig.username,
      mqttConfig.password,
      availTopic.c_str(), 0, true, "offline"
    );
  } else {
    connected = mqttClient.connect(
      mqttConfig.clientId,
      availTopic.c_str(), 0, true, "offline"
    );
  }
  
  if (connected) {
    Serial.println("connected!");
    mqttConfig.connected = true;
    mqttConfig.connectionFailures = 0;
    
    // Publish online status
    mqttClient.publish(availTopic.c_str(), "online", true);
    
    // Force HA discovery republish
    haDiscoveryPublished = false;
    
    return true;
  } else {
    Serial.printf("failed (rc=%d)\n", mqttClient.state());
    mqttConfig.connected = false;
    mqttConfig.connectionFailures++;
    return false;
  }
}

// ===========================================
// HOME ASSISTANT DISCOVERY PUBLISHING
// ===========================================

inline void mqttPublishHADiscovery() {
  if (!mqttConfig.homeAssistantDiscovery || haDiscoveryPublished) {
    return;
  }
  
  if (!mqttClient.connected()) {
    return;
  }
  
  Serial.println("[MQTT] Publishing Home Assistant discovery...");
  
  String topic, payload;
  bool ok;
  
  // Status sensor
  topic = mqttConfig.getHADiscoveryTopic("sensor", "status");
  payload = buildHADiscoveryStatus();
  ok = mqttClient.publish(topic.c_str(), payload.c_str(), true);
  Serial.printf("[MQTT] -> %s (%d bytes) %s\n", topic.c_str(), payload.length(), ok?"OK":"FAIL");
  
  // Binary connectivity sensor
  topic = mqttConfig.getHADiscoveryTopic("binary_sensor", "connectivity");
  payload = buildHADiscoveryConnectivity();
  ok = mqttClient.publish(topic.c_str(), payload.c_str(), true);
  Serial.printf("[MQTT] -> %s (%d bytes) %s\n", topic.c_str(), payload.length(), ok?"OK":"FAIL");
  
  // Uptime sensor
  topic = mqttConfig.getHADiscoveryTopic("sensor", "uptime");
  payload = buildHADiscoveryUptime();
  ok = mqttClient.publish(topic.c_str(), payload.c_str(), true);
  Serial.printf("[MQTT] -> %s (%d bytes) %s\n", topic.c_str(), payload.length(), ok?"OK":"FAIL");
  
  // Success rate sensor
  topic = mqttConfig.getHADiscoveryTopic("sensor", "success_rate");
  payload = buildHADiscoverySuccessRate();
  ok = mqttClient.publish(topic.c_str(), payload.c_str(), true);
  Serial.printf("[MQTT] -> %s (%d bytes) %s\n", topic.c_str(), payload.length(), ok?"OK":"FAIL");
  
  // WiFi RSSI sensor
  topic = mqttConfig.getHADiscoveryTopic("sensor", "rssi");
  payload = buildHADiscoveryRSSI();
  ok = mqttClient.publish(topic.c_str(), payload.c_str(), true);
  Serial.printf("[MQTT] -> %s (%d bytes) %s\n", topic.c_str(), payload.length(), ok?"OK":"FAIL");
  
  // Temperature sensor
  topic = mqttConfig.getHADiscoveryTopic("sensor", "temperature");
  payload = buildHADiscoveryTemperature();
  ok = mqttClient.publish(topic.c_str(), payload.c_str(), true);
  Serial.printf("[MQTT] -> %s (%d bytes) %s\n", topic.c_str(), payload.length(), ok?"OK":"FAIL");
  
  // Failed checks sensor
  topic = mqttConfig.getHADiscoveryTopic("sensor", "failed_checks");
  payload = buildHADiscoveryFailedChecks();
  ok = mqttClient.publish(topic.c_str(), payload.c_str(), true);
  Serial.printf("[MQTT] -> %s (%d bytes) %s\n", topic.c_str(), payload.length(), ok?"OK":"FAIL");
  
  // Downtime sensor
  topic = mqttConfig.getHADiscoveryTopic("sensor", "downtime");
  payload = buildHADiscoveryDowntime();
  ok = mqttClient.publish(topic.c_str(), payload.c_str(), true);
  Serial.printf("[MQTT] -> %s (%d bytes) %s\n", topic.c_str(), payload.length(), ok?"OK":"FAIL");
  
  haDiscoveryPublished = true;
  Serial.println("[MQTT] HA discovery published (8 entities)");
}

// ===========================================
// STATUS PUBLISHING
// ===========================================

inline void mqttPublishStatus() {
  if (!mqttClient.connected()) {
    return;
  }
  
  String topic = mqttConfig.getStateTopic();
  String payload = buildMQTTPayload();
  
  if (mqttClient.publish(topic.c_str(), payload.c_str(), true)) {
    mqttConfig.lastPublishTime = millis();
    lastPublishedState = currentState;
    Serial.printf("[MQTT] Published to %s\n", topic.c_str());
  } else {
    Serial.println("[MQTT] Publish failed!");
  }
}

// ===========================================
// MQTT TASK (runs independently, can block)
// ===========================================

void mqttTask(void* parameter) {
  Serial.printf("[MQTT Task] Started on Core %d\n", xPortGetCoreID());
  
  // Initial delay to let WiFi stabilize
  vTaskDelay(pdMS_TO_TICKS(2000));
  
  while (true) {
    // Only run if MQTT is enabled and WiFi is connected
    if (mqttConfig.enabled && WiFi.status() == WL_CONNECTED) {
      
      // Ensure connected (may block for MQTT_SOCKET_TIMEOUT seconds)
      if (!mqttClient.connected()) {
        mqttConfig.connected = false;
        mqttConnect();
      }
      
      // If connected, do MQTT work
      if (mqttClient.connected()) {
        mqttConfig.connected = true;
        
        // Process incoming messages
        mqttClient.loop();
        
        // Publish HA discovery if needed
        if (mqttConfig.homeAssistantDiscovery && !haDiscoveryPublished) {
          mqttPublishHADiscovery();
        }
        
        // Check if we should publish status
        unsigned long now = millis();
        bool shouldPublish = false;
        
        // Publish on interval
        if (now - mqttConfig.lastPublishTime >= mqttConfig.publishIntervalMs) {
          shouldPublish = true;
        }
        
        // Publish on state change
        if (mqttConfig.publishOnStateChange && currentState != lastPublishedState) {
          shouldPublish = true;
          Serial.println("[MQTT] State changed, publishing immediately");
        }
        
        if (shouldPublish) {
          mqttPublishStatus();
        }
      }
    } else {
      // Not enabled or no WiFi - mark as disconnected
      if (mqttConfig.connected) {
        mqttConfig.connected = false;
      }
    }
    
    // Short delay - FreeRTOS will schedule other tasks
    vTaskDelay(pdMS_TO_TICKS(MQTT_LOOP_DELAY_MS));
  }
}

// ===========================================
// START MQTT TASK
// ===========================================

inline void startMqttTask() {
  if (mqttTaskHandle != NULL) {
    return;  // Already running
  }
  
  xTaskCreatePinnedToCore(
    mqttTask,
    "MQTT",
    MQTT_TASK_STACK_SIZE,
    NULL,
    MQTT_TASK_PRIORITY,
    &mqttTaskHandle,
    MQTT_TASK_CORE
  );
  
  Serial.println("[MQTT] Task created on Core 1");
}

// ===========================================
// MQTT DISCONNECT
// ===========================================

inline void mqttDisconnect() {
  if (mqttClient.connected()) {
    String availTopic = mqttConfig.getAvailabilityTopic();
    mqttClient.publish(availTopic.c_str(), "offline", true);
    mqttClient.disconnect();
  }
  mqttConfig.connected = false;
  Serial.println("[MQTT] Disconnected");
}

// ===========================================
// TEST CONNECTION (for web UI)
// ===========================================

inline bool mqttTestConnection() {
  if (strlen(mqttConfig.broker) == 0) {
    return false;
  }
  
  if (mqttClient.connected()) {
    return true;
  }
  
  // Force immediate connection attempt
  mqttConfig.lastConnectAttempt = 0;
  return mqttConnect();
}

// ===========================================
// RESET HA DISCOVERY
// ===========================================

inline void mqttResetHADiscovery() {
  haDiscoveryPublished = false;
  Serial.println("[MQTT] HA discovery will be republished");
}

// ===========================================
// STOP MQTT TASK (for config changes)
// ===========================================

inline void stopMqttTask() {
  if (mqttTaskHandle != NULL) {
    mqttDisconnect();
    vTaskDelete(mqttTaskHandle);
    mqttTaskHandle = NULL;
    Serial.println("[MQTT] Task stopped");
  }
}

#endif // MQTT_MANAGER_H
