#ifndef SYSTEM_WATCHDOG_H
#define SYSTEM_WATCHDOG_H

/**
 * @file watchdog.h
 * @brief Hardware watchdog timer configuration
 * 
 * Configures ESP32 watchdog to auto-reboot if system becomes unresponsive.
 */

#include <Arduino.h>
#include <esp_task_wdt.h>
#include "../config.h"

// ===========================================
// WATCHDOG SETUP
// ===========================================

inline void setupWatchdog() {
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = WDT_TIMEOUT * 1000,
    .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
    .trigger_panic = true
  };
  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL);  // Add main task
  Serial.println("Watchdog enabled");
}

#endif // SYSTEM_WATCHDOG_H
