#ifndef EFFECT_FIRE_H
#define EFFECT_FIRE_H

#include "effects_base.h"

// Fire color palette thresholds
#define FIRE_DARK_RED_THRESH    0.3f
#define FIRE_ORANGE_THRESH      0.6f
#define FIRE_YELLOW_THRESH      0.85f

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
      
      // Fire color palette
      uint8_t r, g, b;
      if (heat < FIRE_DARK_RED_THRESH) {
        // Black to dark red
        r = (uint8_t)(heat * 3.3f * 180.0f);
        g = 0;
        b = 0;
      } else if (heat < FIRE_ORANGE_THRESH) {
        // Dark red to orange
        float blend = (heat - FIRE_DARK_RED_THRESH) / (FIRE_ORANGE_THRESH - FIRE_DARK_RED_THRESH);
        r = (uint8_t)(180 + blend * 75);
        g = (uint8_t)(blend * 100);
        b = 0;
      } else if (heat < FIRE_YELLOW_THRESH) {
        // Orange to yellow
        float blend = (heat - FIRE_ORANGE_THRESH) / (FIRE_YELLOW_THRESH - FIRE_ORANGE_THRESH);
        r = 255;
        g = (uint8_t)(100 + blend * 155);
        b = 0;
      } else {
        // Yellow to white tips
        float blend = (heat - FIRE_YELLOW_THRESH) / (1.0f - FIRE_YELLOW_THRESH);
        r = 255;
        g = 255;
        b = (uint8_t)(blend * 200);
      }
      
      // Render with row 0 at bottom (flames rise) using consistent API
      setPixelAt(7 - row, col, r, g, b);
    }
  }
  pixels.show();
}

#endif
