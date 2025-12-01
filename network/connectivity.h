#ifndef NETWORK_CONNECTIVITY_H
#define NETWORK_CONNECTIVITY_H

/**
 * @file connectivity.h
 * @brief Internet connectivity checking via HTTP requests
 * 
 * Uses multiple fallback URLs (Google, Cloudflare) for reliability.
 * Implements connection reuse for performance.
 * Returns early on first successful check.
 */

#include <Arduino.h>
#include <HTTPClient.h>
#include <esp_task_wdt.h>
#include "../config.h"

// ===========================================
// REUSABLE HTTP CLIENT
// ===========================================

extern HTTPClient httpClient;
extern bool httpClientInitialized;

// ===========================================
// CONNECTIVITY CHECK FUNCTIONS
// ===========================================

/**
 * Check connectivity to a single URL
 * @param url URL to check (expects 200 or 204 response)
 * @return true if URL responded successfully
 */
inline bool checkSingleUrl(const char* url) {
  // Reuse the global HTTP client for connection pooling
  httpClient.setConnectTimeout(2000);
  httpClient.setTimeout(3000);
  httpClient.setReuse(true);  // Enable connection reuse
  
  if (!httpClient.begin(url)) {
    return false;
  }
  
  int code = httpClient.GET();
  httpClient.end();
  return (code == 204 || code == 200);
}

/**
 * Check internet connectivity by trying multiple URLs
 * Returns early on first success for performance.
 * Resets watchdog between checks to prevent timeout.
 * 
 * @return Number of successful checks (0 = all failed, 1+ = connected)
 */
inline int checkInternet() {
  int successes = 0;
  
  // Try up to 2 URLs, return early on first success
  int maxChecks = min(2, numCheckUrls);
  for (int i = 0; i < maxChecks; i++) {
    esp_task_wdt_reset();
    
    if (checkSingleUrl(checkUrls[i])) {
      successes++;
      esp_task_wdt_reset();
      return successes;  // Early return on success
    }
    
    esp_task_wdt_reset();
  }
  
  return successes;
}

#endif // NETWORK_CONNECTIVITY_H
