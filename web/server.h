#ifndef WEB_SERVER_H
#define WEB_SERVER_H

/**
 * @file server.h
 * @brief Web server setup and route registration
 * 
 * Configures the HTTP server and registers all API endpoint handlers.
 */

#include <Arduino.h>
#include <WebServer.h>
#include "auth.h"
#include "handlers.h"
#include "mqtt_handlers.h"

// ===========================================
// EXTERNAL DECLARATIONS
// ===========================================

extern WebServer server;

// ===========================================
// WEB SERVER SETUP
// ===========================================

inline void setupWebServer() {
  // Collect headers for cookie-based auth
  const char* headerKeys[] = {"Cookie", "Authorization"};
  server.collectHeaders(headerKeys, 2);

  // Core routes
  server.on("/", handleRoot);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/logout", handleLogout);
  server.on("/stats", handleStats);
  server.on("/effect", handleEffect);
  server.on("/brightness", handleBrightness);
  server.on("/rotation", handleRotation);
  server.on("/speed", handleSpeed);
  server.on("/factory-reset", handleFactoryReset);
  
  // MQTT routes
  server.on("/mqtt/config", HTTP_GET, handleMqttGetConfig);
  server.on("/mqtt/config", HTTP_POST, handleMqttSaveConfig);
  server.on("/mqtt/status", HTTP_GET, handleMqttStatus);
  server.on("/mqtt/test", HTTP_POST, handleMqttTest);
  
  server.begin();
  Serial.println("Web server started");
}

#endif // WEB_SERVER_H
