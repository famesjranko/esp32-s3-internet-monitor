#ifndef WEB_PORTAL_H
#define WEB_PORTAL_H

/**
 * @file portal.h
 * @brief WiFi configuration portal (captive portal mode)
 * 
 * Implements the initial setup experience when no WiFi is configured.
 * Creates an AP for users to connect and configure WiFi credentials.
 * Includes network scanning and connection handling.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include "../config.h"
#include "../core/types.h"
#include "../core/state.h"
#include "../storage/nvs_manager.h"
#include "auth.h"
#include "ui_login.h"
#include "ui_portal.h"

// ===========================================
// EXTERNAL DECLARATIONS
// ===========================================

extern WebServer server;
extern DNSServer dnsServer;
extern bool configPortalActive;
extern unsigned long lastPortalActivity;
extern String cachedNetworkListHTML;

// From effects_base.h
extern void fillMatrixImmediate(uint8_t r, uint8_t g, uint8_t b);

// Scan state
static bool scanInProgress = false;

// ===========================================
// NETWORK LIST BUILDER
// ===========================================

inline String buildNetworkListHTML(int networkCount) {
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

// ===========================================
// PORTAL HANDLERS
// ===========================================

inline void handlePortalRoot() {
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

inline void handleScan() {
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

inline void handleConnect() {
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
  saveWebPasswordToNVS(adminPw);

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

    JsonDocument doc;
    doc["success"] = true;
    doc["ip"] = WiFi.localIP().toString();
    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);

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
    
    JsonDocument doc;
    doc["success"] = false;
    doc["error"] = "Connection failed. Check password.";
    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
  }
}

// ===========================================
// PORTAL SETUP
// ===========================================

inline void setupPortalWebServer() {
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

inline void enterConfigMode() {
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
  // Use fixed password for AP mode (only used during initial setup)
  WiFi.softAP(CONFIG_AP_SSID, "admin", CONFIG_AP_CHANNEL);
  delay(500);  // Give AP time to start

  Serial.print("AP SSID: ");
  Serial.println(CONFIG_AP_SSID);
  Serial.print("AP Password: admin");
  Serial.println();
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // Start DNS server - redirect ALL domains to our IP
  dnsServer.start(53, "*", WiFi.softAPIP());

  // Setup portal web server
  setupPortalWebServer();
  server.begin();

  Serial.println("Config portal ready - connect to WiFi: " + String(CONFIG_AP_SSID));
}

#endif // WEB_PORTAL_H
