#ifndef WEB_AUTH_H
#define WEB_AUTH_H

/**
 * @file auth.h
 * @brief Authentication and session management for web interface
 * 
 * Handles login/logout, session tokens, rate limiting, and auth checking.
 * Passwords are verified against SHA-256 hashes stored in NVS.
 */

#include <Arduino.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "../core/types.h"
#include "../core/crypto.h"

// ===========================================
// EXTERNAL DECLARATIONS
// ===========================================

extern WebServer server;
extern AuthState auth;
extern String storedWebPasswordHash;  // SHA-256 hash of web password

// ===========================================
// CONSTANTS
// ===========================================

#define TOKEN_LENGTH 32

// ===========================================
// RESPONSE HELPERS
// ===========================================

/**
 * Send a JSON error response
 * @param code HTTP status code (e.g., 400, 401, 500)
 * @param message Error message to include in response
 */
inline void sendError(int code, const char* message) {
  JsonDocument doc;
  doc["success"] = false;
  doc["error"] = message;
  String output;
  serializeJson(doc, output);
  server.send(code, "application/json", output);
}

/**
 * Send a JSON success response
 * @param message Optional success message
 */
inline void sendSuccess(const char* message = nullptr) {
  JsonDocument doc;
  doc["success"] = true;
  if (message) {
    doc["message"] = message;
  }
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

// ===========================================
// TOKEN GENERATION
// ===========================================

/**
 * Generate a cryptographically random session token
 * Uses ESP32 hardware RNG for better security
 * @return 32-character alphanumeric token
 */
inline String generateToken() {
  String token = "";
  const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  for (int i = 0; i < TOKEN_LENGTH; i++) {
    token += chars[esp_random() % 62];
  }
  return token;
}

// ===========================================
// AUTH CHECKING
// ===========================================

/**
 * Check if current request is authenticated
 * Looks for valid session token in Cookie or Authorization header
 * @return true if authenticated, false otherwise
 */
inline bool checkAuth() {
  // Check session token in cookie or header
  if (server.hasHeader("Cookie")) {
    String cookie = server.header("Cookie");
    int idx = cookie.indexOf("session=");
    if (idx >= 0) {
      String token = cookie.substring(idx + 8);
      int end = token.indexOf(';');
      if (end > 0) token = token.substring(0, end);
      if (token.length() > 0 && token == auth.sessionToken) {
        return true;
      }
    }
  }
  // Also check Authorization header for API calls
  if (server.hasHeader("Authorization")) {
    String authHeader = server.header("Authorization");
    String token = authHeader.substring(7);
    if (token.length() > 0 && token == auth.sessionToken) {
      return true;
    }
  }
  return false;
}

/**
 * Check if login is currently locked out due to too many failed attempts
 * Resets attempt counter after lockout expires
 * @return true if locked out, false if login allowed
 */
inline bool isLockedOut() {
  if (millis() < auth.lockoutUntil) {
    return true;
  }
  if (millis() >= auth.lockoutUntil && auth.loginAttempts >= MAX_LOGIN_ATTEMPTS) {
    auth.loginAttempts = 0;  // Reset after lockout expires
  }
  return false;
}

/**
 * Send 401 Unauthorized response
 */
inline void sendUnauthorized() {
  sendError(401, "unauthorized");
}

// ===========================================
// LOGIN/LOGOUT HANDLERS
// ===========================================

/**
 * Handle POST /login
 * Validates password and creates session token on success
 * Implements rate limiting (5 attempts, 1 min lockout)
 */
inline void handleLogin() {
  if (server.method() != HTTP_POST) {
    sendError(405, "method not allowed");
    return;
  }

  if (isLockedOut()) {
    unsigned long remaining = (auth.lockoutUntil - millis()) / 1000;
    JsonDocument doc;
    doc["success"] = false;
    doc["error"] = "too many attempts";
    doc["retry_after"] = remaining;
    String output;
    serializeJson(doc, output);
    server.send(429, "application/json", output);
    return;
  }

  String password = server.arg("password");

  // Compare hash of entered password with stored hash
  if (verifyPassword(password, storedWebPasswordHash)) {
    auth.loginAttempts = 0;
    auth.sessionToken = generateToken();

    // Max-Age=31536000 = 1 year (browser will remember)
    server.sendHeader("Set-Cookie", "session=" + auth.sessionToken + "; Path=/; HttpOnly; SameSite=Strict; Max-Age=31536000");
    
    JsonDocument doc;
    doc["success"] = true;
    doc["token"] = auth.sessionToken;
    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
  } else {
    auth.loginAttempts++;
    if (auth.loginAttempts >= MAX_LOGIN_ATTEMPTS) {
      auth.lockoutUntil = millis() + LOCKOUT_DURATION;
    }
    JsonDocument doc;
    doc["success"] = false;
    doc["error"] = "invalid password";
    doc["attempts_remaining"] = MAX_LOGIN_ATTEMPTS - auth.loginAttempts;
    String output;
    serializeJson(doc, output);
    server.send(401, "application/json", output);
  }
}

/**
 * Handle GET/POST /logout
 * Clears session token and cookie
 */
inline void handleLogout() {
  auth.sessionToken = "";
  server.sendHeader("Set-Cookie", "session=; Path=/; HttpOnly; Max-Age=0");
  sendSuccess("logged out");
}

#endif // WEB_AUTH_H
