#ifndef EFFECT_INTERFERENCE_H
#define EFFECT_INTERFERENCE_H

#include "effects_base.h"

// Wave source movement parameters
#define INTERF_SOURCE_SPEED_1   0.6f
#define INTERF_SOURCE_SPEED_2   0.7f
#define INTERF_SOURCE_RANGE     3.5f

// Wave parameters
#define INTERF_WAVE_FREQ_1      2.2f
#define INTERF_WAVE_FREQ_2      1.8f
#define INTERF_WAVE_SPEED_1     4.5f
#define INTERF_WAVE_SPEED_2     3.8f

// Constructive interference colors (cyan to white)
#define INTERF_CONST_BASE_R     50
#define INTERF_CONST_BASE_G     150
#define INTERF_CONST_BASE_B     200
#define INTERF_CONST_RANGE_R    200
#define INTERF_CONST_RANGE_G    105
#define INTERF_CONST_RANGE_B    55

// Destructive interference colors (deep blue to purple)
#define INTERF_DESTR_BASE_R     30
#define INTERF_DESTR_BASE_G     20
#define INTERF_DESTR_BASE_B     80
#define INTERF_DESTR_RANGE_R    40
#define INTERF_DESTR_RANGE_G    80
#define INTERF_DESTR_RANGE_B    100

// Effect 14: Interference - Wave interference patterns
void effectInterference() {
  float t = getScaledTime();
  
  // Matrix center
  const float center = (MATRIX_SIZE - 1) / 2.0f;
  
  // Two moving wave sources (fast sin/cos)
  float s1x = center + fastSinF(t * INTERF_SOURCE_SPEED_1) * INTERF_SOURCE_RANGE;
  float s1y = center + fastCosF(t * 0.8f) * INTERF_SOURCE_RANGE;
  float s2x = center + fastSinF(t * INTERF_SOURCE_SPEED_2 + 3.14f) * INTERF_SOURCE_RANGE;
  float s2y = center + fastCosF(t * 0.5f + 2.0f) * INTERF_SOURCE_RANGE;
  
  for (int row = 0; row < MATRIX_SIZE; row++) {
    for (int col = 0; col < MATRIX_SIZE; col++) {
      // Distance from each source (fast sqrt)
      float d1 = fastDist(col - s1x, row - s1y);
      float d2 = fastDist(col - s2x, row - s2y);
      
      // Interference pattern with different frequencies (fast sin)
      float wave1 = fastSinF(d1 * INTERF_WAVE_FREQ_1 - t * INTERF_WAVE_SPEED_1);
      float wave2 = fastSinF(d2 * INTERF_WAVE_FREQ_2 - t * INTERF_WAVE_SPEED_2);
      float interference = wave1 + wave2;  // -2 to +2
      
      // Constructive interference (bright) vs destructive (dark)
      float brightness = (interference + 2.0f) / 4.0f;  // Normalize to 0-1
      
      uint8_t r, g, b;
      
      if (isInternetOK) {
        // Color shifts based on interference type
        if (brightness > 0.5f) {
          // Constructive - cyan to white
          float bright = (brightness - 0.5f) * 2.0f;
          r = INTERF_CONST_BASE_R + (uint8_t)(bright * INTERF_CONST_RANGE_R);
          g = INTERF_CONST_BASE_G + (uint8_t)(bright * INTERF_CONST_RANGE_G);
          b = INTERF_CONST_BASE_B + (uint8_t)(bright * INTERF_CONST_RANGE_B);
        } else {
          // Destructive - deep blue to purple
          float dark = brightness * 2.0f;
          r = INTERF_DESTR_BASE_R + (uint8_t)(dark * INTERF_DESTR_RANGE_R);
          g = INTERF_DESTR_BASE_G + (uint8_t)(dark * INTERF_DESTR_RANGE_G);
          b = INTERF_DESTR_BASE_B + (uint8_t)(dark * INTERF_DESTR_RANGE_B);
        }
      } else {
        // State color with interference modulation
        float intensity = 0.15f + brightness * 0.85f;
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
