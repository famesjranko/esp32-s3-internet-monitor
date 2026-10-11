#ifndef EFFECT_FIRE_H
#define EFFECT_FIRE_H

#include "effects_base.h"

// Flame ramp: black -> state colour at FIRE_BODY_THRESH -> bright tip at heat 1
#define FIRE_BODY_THRESH        0.55f
#define FIRE_TIP_GAIN           2.0f   // Tip is the state colour times this...
#define FIRE_TIP_WHITE          80.0f  // ...plus this much white, so it reads as a hot core

// Fire flicker frequencies
#define FIRE_FLICKER_FREQ_1     1.2f
#define FIRE_FLICKER_FREQ_2     0.7f
#define FIRE_FLICKER_FREQ_3     0.9f
#define FIRE_FLICKER_SPEED_1    8.0f
#define FIRE_FLICKER_SPEED_2    6.0f
#define FIRE_FLICKER_SPEED_3    10.0f

// Heat distribution
#define FIRE_ROW_HEAT_DECAY     0.1f   // Heat decreases 10% per row
#define FIRE_VARIATION_AMP      0.15f  // Variation amplitude

// Maps heat (0..1) to a flame colour built from the fading state colour
inline void fireColor(float heat, uint8_t* r, uint8_t* g, uint8_t* b) {
  if (heat < FIRE_BODY_THRESH) {
    float scale = heat / FIRE_BODY_THRESH;
    *r = (uint8_t)(currentR * scale);
    *g = (uint8_t)(currentG * scale);
    *b = (uint8_t)(currentB * scale);
    return;
  }
  float blend = (heat - FIRE_BODY_THRESH) / (1.0f - FIRE_BODY_THRESH);
  *r = clamp255((int)lerpf(currentR, currentR * FIRE_TIP_GAIN + FIRE_TIP_WHITE, blend));
  *g = clamp255((int)lerpf(currentG, currentG * FIRE_TIP_GAIN + FIRE_TIP_WHITE, blend));
  *b = clamp255((int)lerpf(currentB, currentB * FIRE_TIP_GAIN + FIRE_TIP_WHITE, blend));
}

// Effect 7: Fire - Animated flames filling the matrix
void effectFire() {
  float t = getScaledTime();
  
  for (int row = 0; row < MATRIX_SIZE; row++) {
    for (int col = 0; col < MATRIX_SIZE; col++) {
      // Multiple overlapping sine waves for flickering (using fast sin)
      float flicker1 = fastSinF(col * FIRE_FLICKER_FREQ_1 + t * FIRE_FLICKER_SPEED_1 + row * 0.5f);
      float flicker2 = fastSinF(col * FIRE_FLICKER_FREQ_2 - t * FIRE_FLICKER_SPEED_2 + row * 0.8f);
      float flicker3 = fastSinF((col + row) * FIRE_FLICKER_FREQ_3 + t * FIRE_FLICKER_SPEED_3);
      
      // Combine flickers (normalize from -1..1 to 0..1)
      float flicker = (flicker1 + flicker2 + flicker3 + 3.0f) / 6.0f;
      
      // Heat decreases toward top (row 7 = top when rendered)
      float rowHeat = 1.0f - (row * FIRE_ROW_HEAT_DECAY);
      
      // Add some randomness via position-based variation
      float variation = fastSinF(col * 2.5f + row * 1.8f + t * 5.0f) * FIRE_VARIATION_AMP;
      
      // Final heat value
      float heat = (flicker * 0.6f + rowHeat * 0.4f + variation);
      if (heat < 0.0f) heat = 0.0f;
      if (heat > 1.0f) heat = 1.0f;
      
      uint8_t r, g, b;
      fireColor(heat, &r, &g, &b);

      // Render with row 0 at bottom (flames rise) using consistent API
      setPixelAt(7 - row, col, r, g, b);
    }
  }
  pixels.show();
}

#endif
