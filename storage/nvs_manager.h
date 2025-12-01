#ifndef STORAGE_NVS_MANAGER_H
#define STORAGE_NVS_MANAGER_H

/**
 * @file nvs_manager.h
 * @brief Non-Volatile Storage (NVS) management for persistent settings
 * 
 * Handles loading and saving of:
 * - WiFi credentials (SSID, password)
 * - Web UI password (stored as SHA-256 hash)
 * - Display settings (brightness, effect, speed, rotation)
 * 
 * Implements debounced writes to protect flash from wear.
 */

#include <Arduino.h>
#include <Preferences.h>
#include "../config.h"
#include "../core/types.h"
#include "../core/crypto.h"

// ===========================================
// EXTERNAL DECLARATIONS
// ===========================================

extern Preferences preferences;

// Display settings (from state.h)
extern volatile int currentEffect;
extern volatile uint8_t currentBrightness;
extern volatile uint8_t currentRotation;
extern volatile uint8_t effectSpeed;
extern const uint8_t effectDefaults[NUM_EFFECTS][2];

// Stored credentials
extern String storedSSID;
extern String storedPassword;
extern String storedWebPasswordHash;  // SHA-256 hash of web password

// Settings debounce
extern bool settingsPendingSave;
extern unsigned long lastSettingChangeTime;

// ===========================================
// CREDENTIAL MANAGEMENT
// ===========================================

/**
 * Load WiFi credentials and web password hash from NVS
 * Handles migration from plaintext to hashed passwords
 * @return true if valid WiFi credentials exist, false otherwise
 */
inline bool loadCredentialsFromNVS() {
  preferences.begin(NVS_NAMESPACE, false);  // read-write for migration
  bool configured = preferences.getBool(NVS_KEY_CONFIGURED, false);
  if (configured) {
    storedSSID = preferences.getString(NVS_KEY_SSID, "");
    storedPassword = preferences.getString(NVS_KEY_PASSWORD, "");
  }
  
  // Load password hash (check for migration from plaintext)
  storedWebPasswordHash = preferences.getString(NVS_KEY_WEB_PASS_HASH, "");
  
  if (storedWebPasswordHash.length() == 0) {
    // No hash stored - check for legacy plaintext password
    String legacyPassword = preferences.getString(NVS_KEY_WEB_PASSWORD, "");
    
    if (legacyPassword.length() > 0) {
      // Migrate: hash the plaintext password and store
      Serial.println("Migrating password to SHA-256 hash...");
      storedWebPasswordHash = sha256(legacyPassword);
      preferences.putString(NVS_KEY_WEB_PASS_HASH, storedWebPasswordHash);
      preferences.remove(NVS_KEY_WEB_PASSWORD);  // Remove plaintext
      Serial.println("Password migration complete");
    } else {
      // No password at all - use default
      storedWebPasswordHash = sha256("admin");
      preferences.putString(NVS_KEY_WEB_PASS_HASH, storedWebPasswordHash);
      Serial.println("Default password hash stored");
    }
  }
  
  preferences.end();

  Serial.print("NVS configured: ");
  Serial.println(configured ? "yes" : "no");
  if (configured && storedSSID.length() > 0) {
    Serial.print("NVS SSID: ");
    Serial.println(storedSSID);
  }
  Serial.println("Password hash loaded");

  return configured && storedSSID.length() > 0;
}

/**
 * Save WiFi credentials to NVS
 * @param ssid WiFi network name
 * @param password WiFi password
 */
inline void saveCredentialsToNVS(const String& ssid, const String& password) {
  preferences.begin(NVS_NAMESPACE, false);  // read-write
  preferences.putString(NVS_KEY_SSID, ssid);
  preferences.putString(NVS_KEY_PASSWORD, password);
  preferences.putBool(NVS_KEY_CONFIGURED, true);
  preferences.end();

  Serial.println("Credentials saved to NVS");
}

inline void clearNVSCredentials() {
  preferences.begin(NVS_NAMESPACE, false);
  preferences.remove(NVS_KEY_SSID);
  preferences.remove(NVS_KEY_PASSWORD);
  preferences.putBool(NVS_KEY_CONFIGURED, false);
  preferences.end();
  Serial.println("NVS credentials cleared");
}

/**
 * Save web password to NVS as SHA-256 hash
 * @param password Plain text password (will be hashed before storing)
 */
inline void saveWebPasswordToNVS(const String& password) {
  String hash = sha256(password);
  preferences.begin(NVS_NAMESPACE, false);
  preferences.putString(NVS_KEY_WEB_PASS_HASH, hash);
  preferences.remove(NVS_KEY_WEB_PASSWORD);  // Remove any legacy plaintext
  preferences.end();
  storedWebPasswordHash = hash;  // Update in-memory copy
  Serial.println("Password hash saved to NVS");
}

// ===========================================
// SETTINGS PERSISTENCE
// ===========================================

inline void loadSettingsFromNVS() {
  preferences.begin(NVS_NAMESPACE, true);  // read-only

  // Load effect first (needed to get per-effect defaults)
  if (preferences.isKey(NVS_KEY_EFFECT)) {
    currentEffect = preferences.getUChar(NVS_KEY_EFFECT, currentEffect);
  }
  
  // Apply per-effect defaults, then override with saved values if they exist
  if (currentEffect < NUM_EFFECTS) {
    currentBrightness = effectDefaults[currentEffect][0];
    effectSpeed = effectDefaults[currentEffect][1];
  }
  
  // Override with saved values if they exist
  if (preferences.isKey(NVS_KEY_BRIGHTNESS)) {
    currentBrightness = preferences.getUChar(NVS_KEY_BRIGHTNESS, currentBrightness);
  }
  if (preferences.isKey(NVS_KEY_ROTATION)) {
    currentRotation = preferences.getUChar(NVS_KEY_ROTATION, currentRotation);
  }
  if (preferences.isKey(NVS_KEY_SPEED)) {
    effectSpeed = preferences.getUChar(NVS_KEY_SPEED, effectSpeed);
  }

  preferences.end();

  Serial.print("Loaded settings - Brightness: ");
  Serial.print(currentBrightness);
  Serial.print(", Effect: ");
  Serial.print(currentEffect);
  Serial.print(", Rotation: ");
  Serial.print(currentRotation);
  Serial.print(", Speed: ");
  Serial.println(effectSpeed);
}

inline void markSettingsChanged() {
  settingsPendingSave = true;
  lastSettingChangeTime = millis();
}

inline void saveSettingsToNVSIfNeeded() {
  // Only save if settings changed and debounce time has passed
  if (!settingsPendingSave) return;
  if (millis() - lastSettingChangeTime < NVS_WRITE_DELAY_MS) return;

  preferences.begin(NVS_NAMESPACE, false);  // read-write
  preferences.putUChar(NVS_KEY_BRIGHTNESS, currentBrightness);
  preferences.putUChar(NVS_KEY_EFFECT, currentEffect);
  preferences.putUChar(NVS_KEY_ROTATION, currentRotation);
  preferences.putUChar(NVS_KEY_SPEED, effectSpeed);
  preferences.end();

  settingsPendingSave = false;
  Serial.println("Settings saved to NVS");
}

inline void clearAllNVS() {
  preferences.begin(NVS_NAMESPACE, false);
  preferences.clear();
  preferences.end();
  Serial.println("All NVS data cleared");
}

#endif // STORAGE_NVS_MANAGER_H
