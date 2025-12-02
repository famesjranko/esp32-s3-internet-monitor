#ifndef CORE_TYPES_H
#define CORE_TYPES_H

/**
 * @file types.h
 * @brief Core type definitions for Internet Monitor
 * 
 * Contains all enums, structs, and type definitions used across the project.
 * This file should be included early in the compilation order.
 */

#include <Arduino.h>

// ===========================================
// STATE MACHINE ENUMS
// ===========================================

enum State {
  STATE_BOOTING = 0,
  STATE_CONNECTING_WIFI = 1,
  STATE_CONFIG_PORTAL = 2,
  STATE_WIFI_LOST = 3,
  STATE_INTERNET_OK = 4,
  STATE_INTERNET_DEGRADED = 5,
  STATE_INTERNET_DOWN = 6
};

enum Effect {
  EFFECT_OFF,
  EFFECT_SOLID,
  EFFECT_RIPPLE,
  EFFECT_RAINBOW,
  EFFECT_RAIN,
  EFFECT_MATRIX,
  EFFECT_FIRE,
  EFFECT_PLASMA,
  EFFECT_OCEAN,
  EFFECT_NEBULA,
  EFFECT_LIFE,
  EFFECT_PONG,
  EFFECT_METABALLS,
  EFFECT_INTERFERENCE,
  EFFECT_NOISE,
  EFFECT_RIPPLE_POOL,
  EFFECT_RINGS,
  EFFECT_BALL,
  NUM_EFFECTS  // = 18
};

// ===========================================
// API ERROR CODES
// ===========================================

/**
 * Standard API error codes for consistent error handling
 * Used with sendError() in web handlers
 */
enum APIError {
  API_OK = 0,
  API_ERR_UNAUTHORIZED = 401,
  API_ERR_NOT_FOUND = 404,
  API_ERR_METHOD_NOT_ALLOWED = 405,
  API_ERR_TOO_MANY_REQUESTS = 429,
  API_ERR_INVALID_PARAM = 400,
  API_ERR_SERVER_ERROR = 500
};

// ===========================================
// SYSTEM STATISTICS
// ===========================================

struct SystemStats {
  unsigned long totalChecks = 0;
  unsigned long successfulChecks = 0;
  unsigned long failedChecks = 0;
  int consecutiveFailures = 0;
  int consecutiveSuccesses = 0;
  unsigned long lastDowntime = 0;
  unsigned long totalDowntimeMs = 0;
  unsigned long downtimeStart = 0;
  bool wasDown = false;
  unsigned long bootTime = 0;
};

// ===========================================
// PERFORMANCE METRICS
// ===========================================

struct PerformanceMetrics {
  // LED task metrics
  unsigned long ledFrameCount = 0;
  float ledActualFPS = 60.0f;
  unsigned long ledFrameTimeUs = 0;
  unsigned long ledMaxFrameTimeUs = 0;
  unsigned long ledStackHighWater = 0;
  
  // Network task metrics
  unsigned long netStackHighWater = 0;
};

// ===========================================
// DISPLAY SETTINGS
// ===========================================

struct DisplaySettings {
  uint8_t brightness;
  uint8_t rotation;
  uint8_t speed;
  int effect;
};

// ===========================================
// AUTH STATE
// ===========================================

struct AuthState {
  String sessionToken = "";
  int loginAttempts = 0;
  unsigned long lockoutUntil = 0;
};

// Rate limiting constants
constexpr int MAX_LOGIN_ATTEMPTS = 5;
constexpr unsigned long LOCKOUT_DURATION = 60000;  // 1 minute

// ===========================================
// COLOR STRUCT
// ===========================================

struct RGB {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

#endif // CORE_TYPES_H
