#ifndef EFFECT_RINGS_H
#define EFFECT_RINGS_H

#include "effects_base.h"

// Ring parameters
#define RINGS_WAVE_FREQ       2.0f
#define RINGS_ANIM_SPEED      3.0f
#define RINGS_PULSE_SPEED     2.0f
#define RINGS_PULSE_MIN       0.7f
#define RINGS_PULSE_RANGE     0.3f

// Effect 17: Rings - Expanding rings from center
void effectRings() {
  float t = getScaledTime();
  
  // Matrix center
  const float center = (MATRIX_SIZE - 1) / 2.0f;
  
  // Overall pulsing brightness (fast sin)
  float pulse = (fastSinF(t * RINGS_PULSE_SPEED) + 1.0f) / 2.0f * RINGS_PULSE_RANGE + RINGS_PULSE_MIN;
  
  for (int row = 0; row < MATRIX_SIZE; row++) {
    for (int col = 0; col < MATRIX_SIZE; col++) {
      // Distance from center (fast sqrt)
      float dist = fastDist(col - center, row - center);
      
      // Expanding ring wave (fast sin)
      float ring = fastSinF(dist * RINGS_WAVE_FREQ - t * RINGS_ANIM_SPEED);
      float v = (ring + 1.0f) / 2.0f * pulse;  // Normalize and apply pulse
      
      setPixelAt(row, col,
        (uint8_t)(currentR * v),
        (uint8_t)(currentG * v),
        (uint8_t)(currentB * v)
      );
    }
  }
  pixels.show();
}

#endif
