#ifndef EFFECT_RIPPLE_H
#define EFFECT_RIPPLE_H

#include "effects_base.h"

// Ripple parameters
#define RIPPLE_WAVE_FREQ      1.5f
#define RIPPLE_TIME_DIVISOR   500.0f
#define RIPPLE_MIN_BRIGHTNESS 0.3f
#define RIPPLE_BRIGHTNESS_RANGE 0.7f

// Effect 2: Ripple - Expanding rings from center
void effectRipple() {
  float t = millis() / RIPPLE_TIME_DIVISOR * getTimeScale();
  
  // Matrix center
  const float center = (MATRIX_SIZE - 1) / 2.0f;
  
  for (int row = 0; row < MATRIX_SIZE; row++) {
    for (int col = 0; col < MATRIX_SIZE; col++) {
      // Distance from center (fast sqrt)
      float dist = fastDist(col - center, row - center);
      
      // Expanding wave (fast sin)
      float wave = fastSinF(dist * RIPPLE_WAVE_FREQ - t) * 0.5f + 0.5f;
      
      float brightness = RIPPLE_MIN_BRIGHTNESS + wave * RIPPLE_BRIGHTNESS_RANGE;
      uint8_t r = (uint8_t)(currentR * brightness);
      uint8_t g = (uint8_t)(currentG * brightness);
      uint8_t b = (uint8_t)(currentB * brightness);
      
      setPixelAt(row, col, r, g, b);
    }
  }
  pixels.show();
}

#endif
