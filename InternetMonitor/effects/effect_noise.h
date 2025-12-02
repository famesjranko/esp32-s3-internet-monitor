#ifndef EFFECT_NOISE_H
#define EFFECT_NOISE_H

#include "effects_base.h"

// Noise wave frequencies
#define NOISE_FREQ_1          0.7f
#define NOISE_FREQ_2          0.9f
#define NOISE_FREQ_3          0.4f
#define NOISE_FREQ_4          0.5f
#define NOISE_FREQ_5          0.6f

// Noise animation speeds
#define NOISE_SPEED_1         1.1f
#define NOISE_SPEED_2         0.7f
#define NOISE_SPEED_3         1.3f
#define NOISE_SPEED_4         0.9f
#define NOISE_SPEED_5         0.5f

// Noise mixing weights (should sum to 1.0)
#define NOISE_WEIGHT_1        0.3f
#define NOISE_WEIGHT_2        0.3f
#define NOISE_WEIGHT_3        0.2f
#define NOISE_WEIGHT_4        0.1f
#define NOISE_WEIGHT_5        0.1f

// Brightness limits to prevent saturation
#define NOISE_MIN_BRIGHTNESS  0.05f
#define NOISE_MAX_BRIGHTNESS  0.95f

// Update timing
#define NOISE_BASE_DELAY_MS   40
#define NOISE_Z_INCREMENT     0.08f

// Noise state (static, persists between frames)
static float noiseZ = 0;
static unsigned long noiseLastUpdate = 0;

// Reset function - call when switching to this effect
void resetNoiseEffect() {
  noiseZ = 0;
  noiseLastUpdate = 0;
}

// Effect 15: Noise - Flowing noise field
void effectNoise() {
  unsigned long now = millis();
  float speedMult = getTimeScale();
  
  if (now - noiseLastUpdate > (NOISE_BASE_DELAY_MS / speedMult)) {
    noiseLastUpdate = now;
    noiseZ += NOISE_Z_INCREMENT;
  }
  
  for (int row = 0; row < MATRIX_SIZE; row++) {
    for (int col = 0; col < MATRIX_SIZE; col++) {
      // Multi-frequency noise pattern (fast sin/cos)
      float n1 = fastSinF(col * NOISE_FREQ_1 + noiseZ * NOISE_SPEED_1);
      float n2 = fastSinF(row * NOISE_FREQ_2 + noiseZ * NOISE_SPEED_2);
      float n3 = fastSinF((col + row) * NOISE_FREQ_3 + noiseZ * NOISE_SPEED_3);
      float n4 = fastCosF(col * NOISE_FREQ_4 - row * 0.3f + noiseZ * NOISE_SPEED_4);
      float n5 = fastSinF((col - row) * NOISE_FREQ_5 + noiseZ * NOISE_SPEED_5);
      
      // Weighted combination for more variation
      float combined = n1 * NOISE_WEIGHT_1 + n2 * NOISE_WEIGHT_2 + 
                       n3 * NOISE_WEIGHT_3 + n4 * NOISE_WEIGHT_4 + n5 * NOISE_WEIGHT_5;
      
      // Map to constrained range to prevent full saturation or black
      float v = 0.1f + (combined + 1.0f) * 0.4f;
      if (v < NOISE_MIN_BRIGHTNESS) v = NOISE_MIN_BRIGHTNESS;
      if (v > NOISE_MAX_BRIGHTNESS) v = NOISE_MAX_BRIGHTNESS;
      
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
