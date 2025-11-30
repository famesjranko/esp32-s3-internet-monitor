#ifndef EFFECT_RAIN_H
#define EFFECT_RAIN_H

#include "effects_base.h"

// Rain state (static, persists between frames)
static uint8_t rainDrops[8];      // Current position (0-15, >7 = off screen)
static uint8_t rainSpeeds[8];     // Speed per column (1-3)
static uint8_t rainFrameCount[8]; // Frame counter per column
static unsigned long rainLastUpdate = 0;
static bool rainInitialized = false;

// Reset function - call when switching to this effect
void resetRainEffect() {
  rainInitialized = false;  // Will trigger re-initialization
  rainLastUpdate = 0;
}

// Effect 5: Rain - Random falling drops
void effectRain() {
  // Initialize with random positions and speeds
  if (!rainInitialized) {
    for (int col = 0; col < MATRIX_SIZE; col++) {
      rainDrops[col] = random(12);      // Random start position
      rainSpeeds[col] = 1 + random(3);  // Speed 1-3
      rainFrameCount[col] = random(4);  // Stagger start frames
    }
    rainInitialized = true;
  }
  
  unsigned long now = millis();
  float speedMult = getTimeScale();
  int updateInterval = (int)(50 / speedMult);
  if (updateInterval < 15) updateInterval = 15;
  
  if (now - rainLastUpdate > (unsigned long)updateInterval) {
    rainLastUpdate = now;
    
    for (int col = 0; col < MATRIX_SIZE; col++) {
      rainFrameCount[col]++;
      // Faster drops move more often
      if (rainFrameCount[col] >= (4 - rainSpeeds[col])) {
        rainFrameCount[col] = 0;
        rainDrops[col]++;
        
        // Respawn when off screen with new random speed
        if (rainDrops[col] > 12) {
          rainDrops[col] = 0;
          rainSpeeds[col] = 1 + random(3);
          // Random delay before next drop (0-4 rows off screen)
          if (random(100) < 30) {
            rainDrops[col] = 255 - random(4);  // Will wrap to 0+ soon
          }
        }
      }
    }
  }
  
  // Render drops
  for (int row = 0; row < MATRIX_SIZE; row++) {
    for (int col = 0; col < MATRIX_SIZE; col++) {
      int dropRow = rainDrops[col];
      
      // Check if drop is visible and calculate distance
      if (dropRow < 10) {  // On screen or just below
        int dist = abs(row - (int)dropRow);
        
        if (dist <= 2) {
          // Brighter head, dimmer tail
          float bright = (row <= dropRow) ? 
            (1.0f - dist * 0.3f) :   // Head and body
            (0.5f - dist * 0.2f);    // Trail
          if (bright < 0) bright = 0;
          
          setPixelAt(row, col,
            (uint8_t)(currentR * bright),
            (uint8_t)(currentG * bright),
            (uint8_t)(currentB * bright)
          );
        } else {
          // Background
          setPixelAt(row, col,
            (uint8_t)(currentR * 0.1f),
            (uint8_t)(currentG * 0.1f),
            (uint8_t)(currentB * 0.1f)
          );
        }
      } else {
        // Background for columns with no visible drop
        setPixelAt(row, col,
          (uint8_t)(currentR * 0.1f),
          (uint8_t)(currentG * 0.1f),
          (uint8_t)(currentB * 0.1f)
        );
      }
    }
  }
  pixels.show();
}

#endif
