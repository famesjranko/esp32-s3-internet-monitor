#ifndef EFFECT_METABALLS_H
#define EFFECT_METABALLS_H

#include "effects_base.h"

// Metaball movement speeds
#define META_SPEED_1X     0.7f
#define META_SPEED_1Y     0.9f
#define META_SPEED_2X     0.5f
#define META_SPEED_2Y     0.6f
#define META_SPEED_3X     0.8f
#define META_SPEED_3Y     0.4f

// Metaball influence radius
#define META_RADIUS       0.8f
#define META_SCALE        0.5f   // Prevent saturation

// Edge glow thresholds (where blobs merge)
#define META_EDGE_MIN     0.35f
#define META_EDGE_MAX     0.55f
#define META_EDGE_BOOST   1.5f
#define META_EDGE_ADD     30

// Effect 13: Metaballs - Organic blob merging
void effectMetaballs() {
  float t = getScaledTime();
  
  // Matrix center
  const float center = (MATRIX_SIZE - 1) / 2.0f;
  const float moveRange = 3.0f;
  
  // Three moving blob centers (using fast sin/cos)
  float b1x = center + fastSinF(t * META_SPEED_1X) * moveRange;
  float b1y = center + fastCosF(t * META_SPEED_1Y) * moveRange;
  float b2x = center + fastSinF(t * META_SPEED_2X + 2.0f) * moveRange;
  float b2y = center + fastCosF(t * META_SPEED_2Y + 1.0f) * moveRange;
  float b3x = center + fastSinF(t * META_SPEED_3X + 4.0f) * moveRange;
  float b3y = center + fastCosF(t * META_SPEED_3Y + 3.0f) * moveRange;
  
  for (int row = 0; row < MATRIX_SIZE; row++) {
    for (int col = 0; col < MATRIX_SIZE; col++) {
      // Distance to each blob (using fast sqrt)
      float d1 = fastDist(col - b1x, row - b1y);
      float d2 = fastDist(col - b2x, row - b2y);
      float d3 = fastDist(col - b3x, row - b3y);
      
      // Metaball formula - influence falls off with distance
      float v = META_RADIUS / (d1 + META_RADIUS) + 
                META_RADIUS / (d2 + META_RADIUS) + 
                META_RADIUS / (d3 + META_RADIUS);
      v = v * META_SCALE;  // Scale down to prevent saturation
      if (v > 1.0f) v = 1.0f;
      if (v < 0.0f) v = 0.0f;
      
      uint8_t r = (uint8_t)(currentR * v);
      uint8_t g = (uint8_t)(currentG * v);
      uint8_t b = (uint8_t)(currentB * v);
      
      // Edge glow where blobs meet (creates visible boundaries)
      if (v > META_EDGE_MIN && v < META_EDGE_MAX) {
        r = clamp255((int)(r * META_EDGE_BOOST + META_EDGE_ADD));
        g = clamp255((int)(g * META_EDGE_BOOST + META_EDGE_ADD));
        b = clamp255((int)(b * META_EDGE_BOOST + META_EDGE_ADD));
      }
      
      setPixelAt(row, col, r, g, b);
    }
  }
  pixels.show();
}

#endif
