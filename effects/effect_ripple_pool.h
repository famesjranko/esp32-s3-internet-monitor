#ifndef EFFECT_RIPPLE_POOL_H
#define EFFECT_RIPPLE_POOL_H

#include "effects_base.h"

// Pool/water color constants
#define POOL_BASE_GREEN       30
#define POOL_BASE_BLUE        80
#define POOL_GREEN_RANGE      150
#define POOL_BLUE_RANGE       175

// Ripple center movement speeds
#define POOL_CENTER_SPEED_1   0.7f
#define POOL_CENTER_SPEED_2   0.5f
#define POOL_CENTER_SPEED_3   0.4f

// Ripple wave frequencies
#define POOL_WAVE_FREQ_1      1.8f
#define POOL_WAVE_FREQ_2      1.4f
#define POOL_WAVE_FREQ_3      2.0f

// Ripple animation speeds
#define POOL_ANIM_SPEED_1     3.5f
#define POOL_ANIM_SPEED_2     2.8f
#define POOL_ANIM_SPEED_3     3.2f

// Ripple weights (must sum to ~1.0)
#define POOL_WEIGHT_1         0.4f
#define POOL_WEIGHT_2         0.35f
#define POOL_WEIGHT_3         0.25f

// Effect 16: Ripple Pool - Multiple overlapping ripples
void effectRipplePool() {
  float t = getScaledTime();
  
  // Matrix center point
  const float center = (MATRIX_SIZE - 1) / 2.0f;
  
  // Three ripple centers moving in different patterns (using fast sin/cos)
  float c1x = center + fastSinF(t * POOL_CENTER_SPEED_1) * 2.5f;
  float c1y = center + fastCosF(t * 0.6f) * 2.5f;
  float c2x = center + fastSinF(t * POOL_CENTER_SPEED_2 + 2.1f) * 3.0f;
  float c2y = center + fastCosF(t * 0.4f + 1.5f) * 3.0f;
  float c3x = center + fastSinF(t * POOL_CENTER_SPEED_3 + 4.2f) * 2.0f;
  float c3y = center + fastCosF(t * 0.5f + 3.8f) * 2.0f;
  
  for (int row = 0; row < MATRIX_SIZE; row++) {
    for (int col = 0; col < MATRIX_SIZE; col++) {
      // Calculate distances using fast sqrt (Quake III algorithm)
      float d1 = fastDist(col - c1x, row - c1y);
      float d2 = fastDist(col - c2x, row - c2y);
      float d3 = fastDist(col - c3x, row - c3y);
      
      // Overlapping ripples with different frequencies (fast sin)
      float r1 = fastSinF(d1 * POOL_WAVE_FREQ_1 - t * POOL_ANIM_SPEED_1);
      float r2 = fastSinF(d2 * POOL_WAVE_FREQ_2 - t * POOL_ANIM_SPEED_2);
      float r3 = fastSinF(d3 * POOL_WAVE_FREQ_3 - t * POOL_ANIM_SPEED_3);
      
      // Combine ripples - weighted sum, normalize to 0-1
      float v = (r1 * POOL_WEIGHT_1 + r2 * POOL_WEIGHT_2 + r3 * POOL_WEIGHT_3 + 1.0f) / 2.0f;
      
      uint8_t r, g, b;
      
      if (isInternetOK) {
        // Water - pure blue/cyan, no red to avoid purple
        r = 0;
        g = POOL_BASE_GREEN + (uint8_t)(v * POOL_GREEN_RANGE);
        b = POOL_BASE_BLUE + (uint8_t)(v * POOL_BLUE_RANGE);
        
        // Bright cyan highlights on peaks
        if (v > WATER_HIGHLIGHT_THRESH) {
          float highlight = (v - WATER_HIGHLIGHT_THRESH) / (1.0f - WATER_HIGHLIGHT_THRESH);
          g = g + (uint8_t)((255 - g) * highlight * 0.5f);
          b = b + (uint8_t)((255 - b) * highlight * 0.3f);
        }
      } else {
        // Use state color with ripple modulation
        float intensity = BRIGHTNESS_MIN_RATIO + v * (BRIGHTNESS_MAX_RATIO - BRIGHTNESS_MIN_RATIO);
        r = (uint8_t)(currentR * intensity);
        g = (uint8_t)(currentG * intensity);
        b = (uint8_t)(currentB * intensity);
      }
      
      setPixelAt(row, col, r, g, b);
    }
  }
  pixels.show();
}

#endif
