#ifndef CORE_STATE_H
#define CORE_STATE_H

/**
 * @file state.h
 * @brief Global state management and state machine
 * 
 * Manages the device state machine (booting, connecting, online, etc.)
 * and associated LED colors. Uses volatile variables and mutex for
 * thread-safe cross-core access.
 */

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "types.h"
#include "../config.h"

// ===========================================
// EXTERNAL DECLARATIONS
// ===========================================

// From effects_base.h
extern void setTargetColor(uint8_t r, uint8_t g, uint8_t b);

// ===========================================
// GLOBAL STATE (volatile for cross-core access)
// ===========================================

// Mutex for thread-safe state changes
extern portMUX_TYPE stateMux;

// Current system state
extern volatile int currentState;

// Display settings (volatile for cross-core access)
extern volatile int currentEffect;
extern volatile uint8_t currentBrightness;
extern volatile uint8_t currentRotation;
extern volatile uint8_t effectSpeed;

// Colors for fading (volatile for cross-core access)
extern volatile uint8_t currentR, currentG, currentB;
extern volatile uint8_t targetR, targetG, targetB;
extern uint8_t fadeStartR, fadeStartG, fadeStartB;
extern unsigned long fadeStartTime;

// Status flag for effects
extern volatile bool isInternetOK;

// Task control
extern volatile bool ledTaskRunning;
extern volatile bool ledTaskPaused;

// Task handles
extern TaskHandle_t ledTaskHandle;
extern TaskHandle_t networkTaskHandle;

// Global instances
extern SystemStats stats;
extern PerformanceMetrics perf;
extern AuthState auth;

// Timing
extern unsigned long stateChangeTime;

// Config portal state
extern bool configPortalActive;
extern unsigned long lastPortalActivity;

// ===========================================
// STATE MANAGEMENT FUNCTIONS
// ===========================================

/**
 * Change the system state with thread-safe mutex protection
 * Updates LED colors, tracks downtime, and logs state transitions
 * 
 * @param newState The new state to transition to
 * 
 * States:
 * - STATE_BOOTING: Initial boot (blue)
 * - STATE_CONNECTING_WIFI: Connecting to WiFi (cyan)
 * - STATE_CONFIG_PORTAL: AP mode for configuration (purple)
 * - STATE_WIFI_LOST: WiFi connection lost (red)
 * - STATE_INTERNET_OK: Internet working (green)
 * - STATE_INTERNET_DEGRADED: Partial failures (yellow)
 * - STATE_INTERNET_DOWN: Internet down (red/orange)
 */
inline void changeState(State newState) {
  if (currentState == newState) return;
  
  // Use mutex for thread-safe state changes
  portENTER_CRITICAL(&stateMux);
  
  Serial.print("[State] ");
  Serial.print(currentState);
  Serial.print(" -> ");
  Serial.println(newState);
  
  // Track downtime
  if (newState == STATE_INTERNET_DOWN && !stats.wasDown) {
    stats.downtimeStart = millis();
    stats.wasDown = true;
  } else if (newState == STATE_INTERNET_OK && stats.wasDown) {
    stats.totalDowntimeMs += millis() - stats.downtimeStart;
    stats.lastDowntime = millis() - stats.downtimeStart;
    stats.wasDown = false;
  }
  
  currentState = newState;
  stateChangeTime = millis();
  isInternetOK = (newState == STATE_INTERNET_OK);

  portEXIT_CRITICAL(&stateMux);

  // Set colors based on state (using config.h constants)
  switch (newState) {
    case STATE_BOOTING:
      setTargetColor(COLOR_BOOTING_R, COLOR_BOOTING_G, COLOR_BOOTING_B);
      break;
    case STATE_CONNECTING_WIFI:
      setTargetColor(COLOR_CONNECTING_R, COLOR_CONNECTING_G, COLOR_CONNECTING_B);
      break;
    case STATE_CONFIG_PORTAL:
      setTargetColor(COLOR_PORTAL_R, COLOR_PORTAL_G, COLOR_PORTAL_B);
      break;
    case STATE_WIFI_LOST:
      setTargetColor(COLOR_WIFI_LOST_R, COLOR_WIFI_LOST_G, COLOR_WIFI_LOST_B);
      break;
    case STATE_INTERNET_OK:
      setTargetColor(COLOR_OK_R, COLOR_OK_G, COLOR_OK_B);
      break;
    case STATE_INTERNET_DEGRADED:
      setTargetColor(COLOR_DEGRADED_R, COLOR_DEGRADED_G, COLOR_DEGRADED_B);
      break;
    case STATE_INTERNET_DOWN:
      setTargetColor(COLOR_DOWN_R, COLOR_DOWN_G, COLOR_DOWN_B);
      break;
  }
}

// ===========================================
// HELPER FUNCTIONS
// ===========================================

inline String formatUptime(unsigned long ms) {
  unsigned long seconds = ms / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  unsigned long days = hours / 24;
  
  String result = "";
  if (days > 0) result += String(days) + "d ";
  if (hours % 24 > 0) result += String(hours % 24) + "h ";
  if (minutes % 60 > 0) result += String(minutes % 60) + "m ";
  result += String(seconds % 60) + "s";
  return result;
}

inline float getChipTemp() {
  return temperatureRead();
}

#endif // CORE_STATE_H
