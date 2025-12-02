#ifndef EFFECT_OCEAN_H
#define EFFECT_OCEAN_H

#include "effects_base.h"

// Wave parameters
#define OCEAN_WAVE_FREQ_1     0.9f
#define OCEAN_WAVE_FREQ_2     0.6f
#define OCEAN_WAVE_FREQ_3     0.5f
#define OCEAN_WAVE_SPEED_1    1.8f
#define OCEAN_WAVE_SPEED_2    1.3f
#define OCEAN_WAVE_SPEED_3    0.7f

// Water color (deep blue to light cyan)
#define OCEAN_BASE_R          0
#define OCEAN_BASE_G          30
#define OCEAN_BASE_B          80
#define OCEAN_RANGE_R         60
#define OCEAN_RANGE_G         120
#define OCEAN_RANGE_B         175

// Foam parameters
#define OCEAN_FOAM_THRESH     0.72f
#define OCEAN_FOAM_R_BLEND    0.7f
#define OCEAN_FOAM_G_BLEND    0.7f
#define OCEAN_FOAM_B_BLEND    0.5f

// Effect 9: Ocean - Wave layers with foam
void effectOcean() {
  float t = getScaledTime();
  
  for (int row = 0; row < MATRIX_SIZE; row++) {
    for (int col = 0; col < MATRIX_SIZE; col++) {
      // Three wave layers with variation (fast sin)
      float wave1 = fastSinF(col * OCEAN_WAVE_FREQ_1 + t * OCEAN_WAVE_SPEED_1 + row * 0.4f);
      float wave2 = fastSinF(col * OCEAN_WAVE_FREQ_2 - t * OCEAN_WAVE_SPEED_2 + row * 0.6f);
      float wave3 = fastSinF((col + row) * OCEAN_WAVE_FREQ_3 + t * OCEAN_WAVE_SPEED_3);
      
      float v = (wave1 + wave2 + wave3 + 3.0f) / 6.0f;  // Normalize to 0-1
      
      uint8_t r, g, b;
      
      if (isInternetOK) {
        // Deep blue to light blue gradient
        // Low v = deep water, high v = wave crest
        r = OCEAN_BASE_R + (uint8_t)(v * OCEAN_RANGE_R);
        g = OCEAN_BASE_G + (uint8_t)(v * OCEAN_RANGE_G);
        b = OCEAN_BASE_B + (uint8_t)(v * OCEAN_RANGE_B);
        
        // Foam on wave peaks - blends toward white
        if (v > OCEAN_FOAM_THRESH) {
          float foam = (v - OCEAN_FOAM_THRESH) / (1.0f - OCEAN_FOAM_THRESH);
          r = r + (uint8_t)((255 - r) * foam * OCEAN_FOAM_R_BLEND);
          g = g + (uint8_t)((255 - g) * foam * OCEAN_FOAM_G_BLEND);
          b = b + (uint8_t)((255 - b) * foam * OCEAN_FOAM_B_BLEND);
        }
      } else {
        // Tinted with state color
        float intensity = 0.3f + v * 0.7f;
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
