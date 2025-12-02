#ifndef EFFECT_NEBULA_H
#define EFFECT_NEBULA_H

#include "effects_base.h"

// Nebula wave parameters
#define NEBULA_WAVE_FREQ_1    0.8f
#define NEBULA_WAVE_FREQ_2    0.5f
#define NEBULA_WAVE_FREQ_3    0.4f
#define NEBULA_RADIAL_FREQ    0.8f

// Color parameters
#define NEBULA_BASE_R         40
#define NEBULA_BASE_G         10
#define NEBULA_BASE_B         80
#define NEBULA_RANGE_R        150
#define NEBULA_RANGE_G        50
#define NEBULA_RANGE_B        175

// Star twinkle parameters
#define NEBULA_STAR_THRESH    0.75f
#define NEBULA_STAR_R_BOOST   100
#define NEBULA_STAR_G_BOOST   150
#define NEBULA_STAR_B_BOOST   80
#define NEBULA_TWINKLE_MOD    17
#define NEBULA_TWINKLE_THRESH 3

// Effect 10: Nebula - Space clouds with stars
void effectNebula() {
  // Nebula uses slower time base (1500ms instead of 1000ms)
  float t = millis() / 1500.0f * getTimeScale();
  
  // Matrix center for radial calculation
  const float center = (MATRIX_SIZE - 1) / 2.0f;
  
  for (int row = 0; row < MATRIX_SIZE; row++) {
    for (int col = 0; col < MATRIX_SIZE; col++) {
      // Multiple overlapping waves for nebula effect (fast sin/cos)
      float n1 = fastSinF(col * NEBULA_WAVE_FREQ_1 + t * 1.1f) * fastCosF(row * 0.6f + t * 0.7f);
      float n2 = fastSinF((col + row) * NEBULA_WAVE_FREQ_2 + t * 0.8f);
      float n3 = fastCosF(col * NEBULA_WAVE_FREQ_3 - t * 0.5f) * fastSinF(row * 0.7f + t * 1.2f);
      float dist = fastDist(col - center, row - center);
      float n4 = fastSinF(dist * NEBULA_RADIAL_FREQ - t * 0.4f);
      
      float v = (n1 + n2 + n3 + n4 + 4.0f) / 8.0f;  // Normalize to 0-1
      
      // Second wave for color variation
      float colorWave = (fastSinF(col * 0.5f + t * 0.6f) + fastSinF(row * 0.7f - t * 0.4f) + 2.0f) / 4.0f;
      
      uint8_t r, g, b;
      
      if (isInternetOK) {
        // Shift between purple, blue, and pink
        r = NEBULA_BASE_R + (uint8_t)(v * NEBULA_RANGE_R * (0.5f + colorWave * 0.5f));
        g = NEBULA_BASE_G + (uint8_t)(v * NEBULA_RANGE_G);
        b = NEBULA_BASE_B + (uint8_t)(v * NEBULA_RANGE_B * (1.0f - colorWave * 0.3f));
        
        // Twinkling stars on bright peaks
        if (v > NEBULA_STAR_THRESH) {
          // Use pixel position + time for pseudo-random twinkle
          int twinkle = ((col * 7 + row * 13 + (int)(t * 10)) % NEBULA_TWINKLE_MOD);
          if (twinkle < NEBULA_TWINKLE_THRESH) {
            r = clamp255(r + NEBULA_STAR_R_BOOST);
            g = clamp255(g + NEBULA_STAR_G_BOOST);
            b = clamp255(b + NEBULA_STAR_B_BOOST);
          }
        }
      } else {
        // State color with nebula variation
        float intensity = 0.15f + v * 0.85f;
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
