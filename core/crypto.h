#ifndef CORE_CRYPTO_H
#define CORE_CRYPTO_H

/**
 * @file crypto.h
 * @brief Cryptographic utilities for password hashing
 * 
 * Uses ESP32's built-in mbedtls library for SHA-256 hashing.
 * No additional dependencies required.
 */

#include <Arduino.h>
#include <mbedtls/sha256.h>

// ===========================================
// SHA-256 HASHING
// ===========================================

/**
 * Compute SHA-256 hash of input string
 * @param input String to hash
 * @return 64-character hex string of the hash
 */
inline String sha256(const String& input) {
  uint8_t hash[32];  // SHA-256 produces 32 bytes
  
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);  // 0 = SHA-256 (not SHA-224)
  mbedtls_sha256_update(&ctx, (const unsigned char*)input.c_str(), input.length());
  mbedtls_sha256_finish(&ctx, hash);
  mbedtls_sha256_free(&ctx);
  
  // Convert to hex string
  String result;
  result.reserve(64);
  for (int i = 0; i < 32; i++) {
    char hex[3];
    snprintf(hex, sizeof(hex), "%02x", hash[i]);
    result += hex;
  }
  
  return result;
}

/**
 * Verify a password against a stored hash
 * @param password Plain text password to check
 * @param storedHash Previously stored SHA-256 hash
 * @return true if password matches hash
 */
inline bool verifyPassword(const String& password, const String& storedHash) {
  return sha256(password) == storedHash;
}

/**
 * Check if a string looks like a SHA-256 hash (64 hex chars)
 * Used to detect if stored password is already hashed
 * @param str String to check
 * @return true if it's a valid SHA-256 hex string
 */
inline bool isSHA256Hash(const String& str) {
  if (str.length() != 64) return false;
  
  for (size_t i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
      return false;
    }
  }
  return true;
}

#endif // CORE_CRYPTO_H
