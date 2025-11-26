#ifndef EFFECTS_H
#define EFFECTS_H

#include <Adafruit_NeoPixel.h>
#include "config.h"

// These are defined in main .ino
extern Adafruit_NeoPixel pixels;
extern uint8_t currentR, currentG, currentB;
extern uint8_t targetR, targetG, targetB;
extern uint8_t fadeStartR, fadeStartG, fadeStartB;
extern unsigned long fadeStartTime;
extern uint8_t currentRotation;
extern uint8_t effectSpeed;
extern int currentEffect;

// Pass isOnline flag from main loop instead of checking state directly
extern bool isInternetOK;

// ===========================================
// LED HELPER FUNCTIONS
// ===========================================

// Map logical row/col to physical pixel index based on rotation
int getPixelIndex(int row, int col) {
  int r = row, c = col;
  switch (currentRotation) {
    case 1:  // 90° CW
      r = col;
      c = 7 - row;
      break;
    case 2:  // 180°
      r = 7 - row;
      c = 7 - col;
      break;
    case 3:  // 270° CW
      r = 7 - col;
      c = row;
      break;
  }
  return r * 8 + c;
}

void setTargetColor(uint8_t r, uint8_t g, uint8_t b) {
  if (targetR == r && targetG == g && targetB == b) return;
  
  fadeStartR = currentR;
  fadeStartG = currentG;
  fadeStartB = currentB;
  targetR = r;
  targetG = g;
  targetB = b;
  fadeStartTime = millis();
}

void updateFade() {
  unsigned long elapsed = millis() - fadeStartTime;
  
  if (elapsed >= FADE_DURATION) {
    currentR = targetR;
    currentG = targetG;
    currentB = targetB;
  } else {
    float progress = (float)elapsed / FADE_DURATION;
    // Ease in-out
    progress = progress < 0.5 ? 2 * progress * progress : 1 - pow(-2 * progress + 2, 2) / 2;
    
    currentR = fadeStartR + (targetR - fadeStartR) * progress;
    currentG = fadeStartG + (targetG - fadeStartG) * progress;
    currentB = fadeStartB + (targetB - fadeStartB) * progress;
  }
}

void fillMatrixImmediate(uint8_t r, uint8_t g, uint8_t b) {
  currentR = targetR = r;
  currentG = targetG = g;
  currentB = targetB = b;
  for (int i = 0; i < NUM_LEDS; i++) {
    pixels.setPixelColor(i, pixels.Color(r, g, b));
  }
  pixels.show();
}

// ===========================================
// LED EFFECTS
// ===========================================

// --- Effect: Off ---
void effectOff() {
  pixels.clear();
  pixels.show();
}

// --- Effect: Solid color (no animation) ---
void effectSolid() {
  for (int i = 0; i < NUM_LEDS; i++) {
    pixels.setPixelColor(i, pixels.Color(currentR, currentG, currentB));
  }
  pixels.show();
}

// --- Effect: Diagonal ripple wave ---
void effectRipple() {
  unsigned long now = millis();
  float speedMult = effectSpeed / 50.0;
  float timeScale = now / 1000.0 * speedMult;
  
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      int i = getPixelIndex(row, col);
      float distance = (row + col) / 14.0;
      float wave = sin((distance * 4.0) - (timeScale * 2.0));
      wave = (wave + 1.0) / 2.0;
      
      // Accent color based on state
      uint8_t r2, g2, b2;
      if (currentG > currentR && currentG > currentB) {
        r2 = 0; g2 = currentG * 0.6; b2 = currentG * 0.6;  // Cyan
      } else if (currentR > currentG && currentR > currentB) {
        r2 = currentR; g2 = currentR * 0.3; b2 = 0;  // Orange
      } else if (currentB > currentR && currentB > currentG) {
        r2 = currentB * 0.4; g2 = 0; b2 = currentB;  // Purple
      } else {
        r2 = currentR; g2 = currentG * 0.4; b2 = 0;  // Amber
      }
      
      uint8_t r = currentR + (int)(r2 - currentR) * wave;
      uint8_t g = currentG + (int)(g2 - currentG) * wave;
      uint8_t b = currentB + (int)(b2 - currentB) * wave;
      
      pixels.setPixelColor(i, pixels.Color(r, g, b));
    }
  }
  pixels.show();
}

// --- Effect: Rainbow wave (adapts to connection status) ---
void effectRainbow() {
  unsigned long now = millis();
  float speedMult = effectSpeed / 50.0;

  if (isInternetOK) {
    // Online: Full vibrant rainbow
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        int i = getPixelIndex(row, col);
        uint16_t hue = (row + col) * 4096 + (uint32_t)(now * 20 * speedMult);
        uint32_t color = pixels.ColorHSV(hue, 255, 70);
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        pixels.setPixelColor(i, pixels.Color(r, g, b));
      }
    }
  } else {
    // Degraded/Offline: Same as ripple but reversed direction
    float timeScale = now / 1000.0 * speedMult;
    
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        int i = getPixelIndex(row, col);
        float distance = (row + col) / 14.0;
        float wave = sin((distance * 4.0) + (timeScale * 2.0));
        wave = (wave + 1.0) / 2.0;
        
        uint8_t r2, g2, b2;
        if (currentG > currentR && currentG > currentB) {
          r2 = 0; g2 = currentG * 0.6; b2 = currentG * 0.6;
        } else if (currentR > currentG && currentR > currentB) {
          r2 = currentR; g2 = currentR * 0.3; b2 = 0;
        } else if (currentB > currentR && currentB > currentG) {
          r2 = currentB * 0.4; g2 = 0; b2 = currentB;
        } else {
          r2 = currentR; g2 = currentG * 0.4; b2 = 0;
        }
        
        uint8_t r = currentR + (int)(r2 - currentR) * wave;
        uint8_t g = currentG + (int)(g2 - currentG) * wave;
        uint8_t b = currentB + (int)(b2 - currentB) * wave;
        
        pixels.setPixelColor(i, pixels.Color(r, g, b));
      }
    }
  }
  pixels.show();
}

// --- Effect: Gentle pulse/breathe between two colors ---
void effectPulse() {
  unsigned long now = millis();
  float speedMult = effectSpeed / 50.0;
  float cycleTime = 3000.0 / speedMult;
  float phase = fmod(now, cycleTime) / cycleTime;
  float blend = (sin(phase * 6.2832) + 1.0) / 2.0;  // 0.0 to 1.0

  // Accent color based on state (same as ripple)
  uint8_t r2, g2, b2;
  if (currentG > currentR && currentG > currentB) {
    // Green (online) -> cyan accent
    r2 = 0; g2 = currentG * 0.6; b2 = currentG * 0.6;
  } else if (currentR > currentG && currentR > currentB) {
    // Red/orange (offline/error) -> orange accent
    r2 = currentR; g2 = currentR * 0.3; b2 = 0;
  } else if (currentB > currentR && currentB > currentG) {
    // Blue (booting) -> purple accent
    r2 = currentB * 0.4; g2 = 0; b2 = currentB;
  } else {
    // Yellow (degraded) -> amber accent
    r2 = currentR; g2 = currentG * 0.4; b2 = 0;
  }

  // Blend between main color and accent
  uint8_t r = currentR + (int)(r2 - currentR) * blend;
  uint8_t g = currentG + (int)(g2 - currentG) * blend;
  uint8_t b = currentB + (int)(b2 - currentB) * blend;

  // Also add subtle brightness variation
  float breath = 0.7 + 0.3 * blend;
  r *= breath;
  g *= breath;
  b *= breath;

  for (int i = 0; i < NUM_LEDS; i++) {
    pixels.setPixelColor(i, pixels.Color(r, g, b));
  }
  pixels.show();
}

// --- Effect: Rain drops falling ---
void effectRain() {
  static uint8_t drops[8] = {0, 2, 5, 1, 7, 3, 6, 4};
  static unsigned long lastUpdate = 0;
  
  unsigned long now = millis();
  float speedMult = effectSpeed / 50.0;
  int updateInterval = 150 / speedMult;
  if (updateInterval < 30) updateInterval = 30;
  
  if (now - lastUpdate > updateInterval) {
    lastUpdate = now;
    for (int col = 0; col < 8; col++) {
      drops[col] = (drops[col] + 1) % 12;
    }
  }
  
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      int i = getPixelIndex(row, col);
      
      int dropRow = drops[col];
      int dist = abs(row - dropRow);
      
      if (dropRow < 8 && dist <= 2) {
        float bright = 1.0 - (dist * 0.35);
        pixels.setPixelColor(i, pixels.Color(
          currentR * bright,
          currentG * bright,
          currentB * bright
        ));
      } else {
        pixels.setPixelColor(i, pixels.Color(
          currentR * 0.15,
          currentG * 0.15,
          currentB * 0.15
        ));
      }
    }
  }
  pixels.show();
}

// --- Apply current effect ---
void applyEffect() {
  switch (currentEffect) {
    case 0: effectOff(); break;
    case 1: effectSolid(); break;
    case 2: effectRipple(); break;
    case 3: effectRainbow(); break;
    case 4: effectPulse(); break;
    case 5: effectRain(); break;
    default: effectSolid(); break;
  }
}

#endif
