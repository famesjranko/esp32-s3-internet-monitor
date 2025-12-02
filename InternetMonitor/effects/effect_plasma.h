#ifndef EFFECT_PLASMA_H
#define EFFECT_PLASMA_H

#include "effects_base.h"

// Plasma wave parameters
#define PLASMA_FREQ_1     0.5f
#define PLASMA_FREQ_2     0.5f
#define PLASMA_FREQ_3     0.5f
#define PLASMA_SPEED_2    0.7f

// Color parameters
#define PLASMA_SATURATION 1.0f
#define PLASMA_VALUE      0.9f

// Effect 8: Plasma - Flowing color blobs
void effectPlasma() {
  float t = getScaledTime();
  
  // Matrix center for radial component
  const float center = (MATRIX_SIZE - 1) / 2.0f;
  
  for (int row = 0; row < MATRIX_SIZE; row++) {
    for (int col = 0; col < MATRIX_SIZE; col++) {
      // Three overlapping waves (using fast sin)
      float v = fastSinF(col * PLASMA_FREQ_1 + t);
      v += fastSinF((row + col) * PLASMA_FREQ_2 + t * PLASMA_SPEED_2);
      v += fastSinF(fastDist(col - center, row - center) * PLASMA_FREQ_3 + t);
      v = (v + 3.0f) / 6.0f;  // Normalize to 0-1
      
      uint8_t r, g, b;
      
      if (isInternetOK) {
        // Full color plasma - cycle through hue
        hsvToRgb(v * 360.0f, PLASMA_SATURATION, PLASMA_VALUE, &r, &g, &b);
      } else {
        // Tinted with state color
        r = (uint8_t)(currentR * v);
        g = (uint8_t)(currentG * v);
        b = (uint8_t)(currentB * v);
      }
      
      setPixelAt(row, col, r, g, b);
    }
  }
  pixels.show();
}

#endif
