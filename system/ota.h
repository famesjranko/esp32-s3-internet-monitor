#ifndef SYSTEM_OTA_H
#define SYSTEM_OTA_H

/**
 * @file ota.h
 * @brief Over-The-Air firmware update support
 * 
 * Configures ArduinoOTA for wireless firmware updates.
 * Shows progress on LED matrix during upload.
 */

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <Adafruit_NeoPixel.h>
#include <esp_task_wdt.h>
#include "../config.h"

// ===========================================
// EXTERNAL DECLARATIONS
// ===========================================

extern Adafruit_NeoPixel pixels;
extern volatile bool ledTaskPaused;

// From effects_base.h
extern void fillMatrixImmediate(uint8_t r, uint8_t g, uint8_t b);

// ===========================================
// OTA SETUP
// ===========================================

// Fixed OTA password - simple and predictable for local network use
#define OTA_PASSWORD "internet-monitor"

/**
 * Initialize OTA firmware updates
 * Password is fixed to "internet-monitor" for ease of use
 */
inline void setupOTA() {
  ArduinoOTA.setHostname("internet-monitor");
  ArduinoOTA.setPassword(OTA_PASSWORD);
  
  ArduinoOTA.onStart([]() {
    Serial.println("OTA starting...");
    // Pause LED task during OTA
    ledTaskPaused = true;
    vTaskDelay(pdMS_TO_TICKS(50));  // Wait for current frame
    // Disable watchdog during OTA to prevent resets
    esp_task_wdt_delete(NULL);
    fillMatrixImmediate(40, 0, 40);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("OTA complete!");
    fillMatrixImmediate(0, 40, 40);
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    int pct = progress / (total / 100);
    int ledsOn = (pct * NUM_LEDS) / 100;
    for (int i = 0; i < NUM_LEDS; i++) {
      pixels.setPixelColor(i, i < ledsOn ? pixels.Color(40, 0, 40) : pixels.Color(5, 0, 5));
    }
    pixels.show();
    yield();  // Let system tasks run
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]\n", error);
    fillMatrixImmediate(50, 0, 0);
    ledTaskPaused = false;  // Resume on error
    // Re-enable watchdog
    esp_task_wdt_add(NULL);
  });
  
  ArduinoOTA.begin();
  Serial.println("OTA ready");
}

#endif // SYSTEM_OTA_H
