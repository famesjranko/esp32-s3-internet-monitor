#ifndef SYSTEM_FACTORY_RESET_H
#define SYSTEM_FACTORY_RESET_H

/**
 * @file factory_reset.h
 * @brief Hardware factory reset via BOOT button
 * 
 * Hold BOOT button for 5 seconds during normal operation
 * to clear all NVS settings and reboot into config portal.
 */

#include <Arduino.h>
#include <Preferences.h>
#include <esp_task_wdt.h>
#include "../config.h"
#include "../effects/effects_base.h"

// ===========================================
// EXTERNAL REFERENCES
// ===========================================

extern Preferences preferences;
extern Adafruit_NeoPixel pixels;
extern volatile bool ledTaskPaused;

// ===========================================
// STATE
// ===========================================

static volatile unsigned long bootButtonPressStart = 0;
static volatile bool bootButtonHeld = false;
static int factoryResetLastSecond = -1;

// ===========================================
// INITIALIZATION
// ===========================================

/**
 * @brief Initialize BOOT button for factory reset detection
 * Call this in setup() after pixels.begin()
 */
inline void initFactoryResetButton() {
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
}

// ===========================================
// MAIN CHECK FUNCTION
// ===========================================

/**
 * @brief Check for hardware factory reset via BOOT button
 * 
 * Call this from the main loop. If BOOT button (GPIO0) is held for 5 seconds
 * during normal operation, performs a factory reset.
 * 
 * Visual feedback:
 * - Red rings fill inward as countdown progresses
 * - Full red matrix when triggered
 * - Green flash on success before reboot
 */
inline void checkBootButtonFactoryReset() {
  bool buttonPressed = (digitalRead(BOOT_BUTTON_PIN) == LOW);
  
  if (buttonPressed && !bootButtonHeld) {
    // Button just pressed - start countdown
    bootButtonHeld = true;
    bootButtonPressStart = millis();
    factoryResetLastSecond = -1;
    ledTaskPaused = true;  // Take over LED control
    Serial.println("\n[Factory Reset] BOOT button pressed - hold for 5 seconds to reset...");
  }
  else if (!buttonPressed && bootButtonHeld) {
    // Button released - cancel
    bootButtonHeld = false;
    ledTaskPaused = false;  // Resume normal LED operation
    unsigned long heldTime = millis() - bootButtonPressStart;
    Serial.printf("[Factory Reset] Released after %lu ms - cancelled\n", heldTime);
    bootButtonPressStart = 0;
  }
  else if (buttonPressed && bootButtonHeld) {
    // Button still held - update progress
    unsigned long heldTime = millis() - bootButtonPressStart;
    float progress = (float)heldTime / FACTORY_RESET_HOLD_TIME;
    if (progress > 1.0f) progress = 1.0f;
    
    // Update visual feedback (effect defined in effects_base.h)
    showFactoryResetProgress(progress);
    
    // Log countdown (once per second)
    int secondsRemaining = (FACTORY_RESET_HOLD_TIME - heldTime) / 1000;
    if (secondsRemaining != factoryResetLastSecond && secondsRemaining >= 0) {
      factoryResetLastSecond = secondsRemaining;
      Serial.printf("[Factory Reset] %d seconds remaining...\n", secondsRemaining + 1);
    }
    
    // Check if held long enough
    if (heldTime >= FACTORY_RESET_HOLD_TIME) {
      Serial.println("\n*** FACTORY RESET TRIGGERED ***");
      
      // Full red matrix
      for (int p = 0; p < NUM_LEDS; p++) {
        pixels.setPixelColor(p, pixels.Color(255, 0, 0));
      }
      pixels.show();
      delay(500);
      
      // Disable watchdog during NVS operations
      esp_task_wdt_delete(NULL);
      
      // Clear all NVS data
      Serial.println("Clearing NVS...");
      preferences.begin(NVS_NAMESPACE, false);
      preferences.clear();
      preferences.end();
      
      // Also clear MQTT namespace
      preferences.begin("mqtt", false);
      preferences.clear();
      preferences.end();
      
      Serial.println("NVS cleared successfully!");
      
      // Success indication: green
      for (int p = 0; p < NUM_LEDS; p++) {
        pixels.setPixelColor(p, pixels.Color(0, 255, 0));
      }
      pixels.show();
      delay(1000);
      
      Serial.println("Rebooting into config portal...\n");
      delay(500);
      ESP.restart();
    }
  }
}

#endif // SYSTEM_FACTORY_RESET_H
