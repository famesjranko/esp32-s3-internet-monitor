#ifndef SYSTEM_TASKS_H
#define SYSTEM_TASKS_H

/**
 * @file tasks.h
 * @brief FreeRTOS task management for dual-core operation
 * 
 * Architecture:
 * - Core 0: LED task (60fps, high priority, never blocks)
 * - Core 1: Network task (internet checks, lower priority)
 * - Core 1: MQTT task (runs separately, see mqtt_manager.h)
 * - Main loop: Web server, OTA (runs on Core 1)
 */

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include "../config.h"
#include "../core/types.h"
#include "../core/state.h"
#include "../network/connectivity.h"

// ===========================================
// DUAL CORE CONFIGURATION
// ===========================================

#define LED_CORE 0          // Core for LED effects (smooth animation)
#define NETWORK_CORE 1      // Core for network operations
#define LED_TASK_PRIORITY 2 // Higher priority for smooth animation
#define NET_TASK_PRIORITY 1 // Lower priority for network

// Performance logging interval
#define PERF_LOG_INTERVAL 5000  // Log every 5 seconds

// ===========================================
// EXTERNAL DECLARATIONS
// ===========================================

extern TaskHandle_t ledTaskHandle;
extern TaskHandle_t networkTaskHandle;
extern SystemStats stats;
extern PerformanceMetrics perf;

extern volatile bool ledTaskRunning;
extern volatile bool ledTaskPaused;
extern volatile int currentState;
extern bool configPortalActive;

// Timing
extern unsigned long lastCheck;

// From effects_base.h / effects.h
extern void updateFade();
extern void applyEffect();

// ===========================================
// LED TASK (Core 0) - Smooth 60fps animation
// ===========================================

/**
 * LED rendering task - runs on Core 0
 * Maintains 60fps with precise timing via vTaskDelayUntil.
 * Updates fade transitions and applies current effect.
 * Reports FPS and frame timing every 5 seconds.
 * 
 * @param parameter Unused task parameter
 */
inline void ledTask(void* parameter) {
  const TickType_t frameDelay = pdMS_TO_TICKS(16);  // ~60fps (16.67ms per frame)
  TickType_t lastWakeTime = xTaskGetTickCount();
  
  // FPS tracking variables
  unsigned long frameCount = 0;
  unsigned long lastFPSReport = millis();
  unsigned long frameStartUs;
  unsigned long maxFrameUs = 0;
  
  Serial.println("[LED Task] Started on Core " + String(xPortGetCoreID()));
  
  // Add this task to watchdog
  esp_task_wdt_add(NULL);
  
  while (ledTaskRunning) {
    esp_task_wdt_reset();
    frameStartUs = micros();
    
    if (!ledTaskPaused) {
      // Update fade and apply effect
      updateFade();
      applyEffect();
    }
    
    // Measure frame time
    unsigned long frameUs = micros() - frameStartUs;
    perf.ledFrameTimeUs = frameUs;
    if (frameUs > maxFrameUs) maxFrameUs = frameUs;
    
    frameCount++;
    perf.ledFrameCount++;
    
    // Report FPS every 5 seconds
    unsigned long now = millis();
    if (now - lastFPSReport >= PERF_LOG_INTERVAL) {
      float fps = (float)frameCount * 1000.0 / (now - lastFPSReport);
      perf.ledActualFPS = fps;
      perf.ledMaxFrameTimeUs = maxFrameUs;
      perf.ledStackHighWater = uxTaskGetStackHighWaterMark(NULL);
      
      Serial.printf("[LED] FPS: %.1f | Frame: %lu us (max %lu us) | Stack: %lu bytes free\n", 
        fps, perf.ledFrameTimeUs, maxFrameUs, perf.ledStackHighWater * 4);
      
      frameCount = 0;
      maxFrameUs = 0;
      lastFPSReport = now;
    }
    
    // Use vTaskDelayUntil for precise timing
    vTaskDelayUntil(&lastWakeTime, frameDelay);
  }
  
  esp_task_wdt_delete(NULL);
  vTaskDelete(NULL);
}

// ===========================================
// NETWORK TASK (Core 1) - Internet checking
// ===========================================

/**
 * Network monitoring task - runs on Core 1
 * Checks internet connectivity at CHECK_INTERVAL (default 10s).
 * Updates system state based on consecutive failures.
 * Reports check statistics every 5 seconds.
 * 
 * @param parameter Unused task parameter
 */
inline void networkTask(void* parameter) {
  const TickType_t checkDelay = pdMS_TO_TICKS(100);  // Check every 100ms
  
  // Performance tracking
  unsigned long lastPerfReport = millis();
  unsigned long checkCount = 0;
  unsigned long totalCheckTimeMs = 0;
  
  Serial.println("[Network Task] Started on Core " + String(xPortGetCoreID()));
  
  // Add this task to watchdog
  esp_task_wdt_add(NULL);
  
  while (true) {
    esp_task_wdt_reset();
    
    // Update stack high water mark periodically
    unsigned long now = millis();
    if (now - lastPerfReport >= PERF_LOG_INTERVAL) {
      perf.netStackHighWater = uxTaskGetStackHighWaterMark(NULL);
      
      if (checkCount > 0) {
        Serial.printf("[Net] Checks: %lu | Avg time: %lu ms | Stack: %lu bytes free\n",
          checkCount, totalCheckTimeMs / checkCount, perf.netStackHighWater * 4);
      }
      
      checkCount = 0;
      totalCheckTimeMs = 0;
      lastPerfReport = now;
    }
    
    // Skip if in config portal mode
    if (configPortalActive) {
      vTaskDelay(checkDelay);
      continue;
    }
    
    // Check WiFi status
    if (WiFi.status() != WL_CONNECTED) {
      if (currentState != STATE_WIFI_LOST && currentState != STATE_CONNECTING_WIFI && 
          currentState != STATE_CONFIG_PORTAL && currentState != STATE_BOOTING) {
        Serial.println("[Network] WiFi lost!");
        changeState(STATE_WIFI_LOST);
      }
      vTaskDelay(checkDelay);
      continue;
    }
    
    // WiFi recovered
    if (currentState == STATE_WIFI_LOST) {
      Serial.println("[Network] WiFi recovered");
      changeState(STATE_INTERNET_OK);
    }
    
    // Internet check (non-blocking timing)
    now = millis();
    if (now - lastCheck >= CHECK_INTERVAL) {
      lastCheck = now;
      
      unsigned long checkStart = millis();
      Serial.print("[Network] Checking... ");
      int successes = checkInternet();
      unsigned long checkTime = millis() - checkStart;
      
      checkCount++;
      totalCheckTimeMs += checkTime;
      
      stats.totalChecks++;
      
      if (successes >= 1) {
        Serial.printf("OK (%lu ms)\n", checkTime);
        stats.successfulChecks++;
        stats.consecutiveFailures = 0;
        stats.consecutiveSuccesses++;
        
        if (currentState != STATE_INTERNET_OK) {
          changeState(STATE_INTERNET_OK);
        }
      } else {
        Serial.printf("FAIL (%lu ms)\n", checkTime);
        stats.failedChecks++;
        stats.consecutiveFailures++;
        stats.consecutiveSuccesses = 0;
        
        if (stats.consecutiveFailures >= FAILURES_BEFORE_RED) {
          changeState(STATE_INTERNET_DOWN);
        } else {
          changeState(STATE_INTERNET_DEGRADED);
        }
      }
    }
    
    vTaskDelay(checkDelay);
  }
}

// ===========================================
// TASK CREATION
// ===========================================

/**
 * Create and start the LED rendering task on Core 0
 * Must be called during setup after pixels.begin()
 */
inline void startLEDTask() {
  xTaskCreatePinnedToCore(
    ledTask,              // Task function
    "LEDTask",            // Task name
    4096,                 // Stack size (bytes)
    NULL,                 // Task parameters
    LED_TASK_PRIORITY,    // Priority (higher = more important)
    &ledTaskHandle,       // Task handle
    LED_CORE              // Core to run on (0)
  );
  Serial.println("LED task created on Core 0");
}

/**
 * Create and start the network monitoring task on Core 1
 * Must be called after WiFi is connected
 */
inline void startNetworkTask() {
  xTaskCreatePinnedToCore(
    networkTask,          // Task function
    "NetworkTask",        // Task name
    8192,                 // Stack size (bytes) - larger for HTTP
    NULL,                 // Task parameters
    NET_TASK_PRIORITY,    // Priority
    &networkTaskHandle,   // Task handle
    NETWORK_CORE          // Core to run on (1)
  );
  Serial.println("Network task created on Core 1");
}

#endif // SYSTEM_TASKS_H
