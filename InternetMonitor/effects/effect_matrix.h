#ifndef EFFECT_MATRIX_H
#define EFFECT_MATRIX_H

#include "effects_base.h"

// Matrix state (static, persists between frames)
static uint8_t matrixColumns[8];
static uint8_t matrixSpeeds[8];
static uint8_t matrixLengths[8];
static uint8_t matrixFrameCount[8];
static unsigned long matrixLastUpdate = 0;
static bool matrixInitialized = false;

// Reset function - call when switching to this effect
void resetMatrixEffect() {
  matrixInitialized = false;  // Will trigger re-initialization
  matrixLastUpdate = 0;
}

// Effect 6: Matrix - Falling code streams
void effectMatrix() {
  if (!matrixInitialized) {
    for (int i = 0; i < MATRIX_SIZE; i++) {
      matrixColumns[i] = random(12);
      matrixSpeeds[i] = 1 + random(3);
      matrixLengths[i] = 2 + random(4);
      matrixFrameCount[i] = 0;
    }
    matrixInitialized = true;
  }
  
  unsigned long now = millis();
  float speedMult = getTimeScale();
  
  if (now - matrixLastUpdate > (60 / speedMult)) {
    matrixLastUpdate = now;
    
    for (int col = 0; col < MATRIX_SIZE; col++) {
      matrixFrameCount[col]++;
      if (matrixFrameCount[col] >= (4 - matrixSpeeds[col])) {
        matrixFrameCount[col] = 0;
        matrixColumns[col]++;
        if (matrixColumns[col] > MATRIX_SIZE + matrixLengths[col]) {
          matrixColumns[col] = 0;
          matrixSpeeds[col] = 1 + random(3);
          matrixLengths[col] = 2 + random(4);
        }
      }
    }
  }
  
  pixels.clear();
  for (int col = 0; col < MATRIX_SIZE; col++) {
    for (int t = 0; t < matrixLengths[col]; t++) {
      int row = matrixColumns[col] - t;
      if (row >= 0 && row < MATRIX_SIZE) {
        if (t == 0) {
          // Bright head - white/green tinted with state
          setPixelAt(row, col,
            (180 + currentR) / 2,
            (255 + currentG) / 2,
            (180 + currentB) / 2
          );
        } else {
          // Tail - matrix green tinted with state
          float fade = 1.0f - (float)t / matrixLengths[col];
          uint8_t g = (uint8_t)(200 * fade);
          setPixelAt(row, col,
            (uint8_t)((currentR * fade) / 3),
            (uint8_t)((g + currentG * fade) / 2),
            (uint8_t)((currentB * fade) / 3)
          );
        }
      }
    }
  }
  
  pixels.show();
}

#endif
