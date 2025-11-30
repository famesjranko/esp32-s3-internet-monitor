#ifndef EFFECT_PULSE_H
#define EFFECT_PULSE_H

#include "effects_base.h"

// Pulse breathing parameters
#define PULSE_SPEED           1.5f
#define PULSE_MIN_BRIGHTNESS  0.3f
#define PULSE_BRIGHTNESS_RANGE 0.7f

// Color shift amount
#define PULSE_GREEN_SHIFT     0.3f   // Green -> cyan shift
#define PULSE_RED_SHIFT       0.2f   // Red -> orange shift
#define PULSE_PHASE_OFFSET    1.0f   // Phase offset for color shift

// Effect 4: Pulse - Smooth breathing effect
void effectPulse() {
  float t = getScaledTime();
  
  // Smooth sine wave breathing (fast sin)
  float breath = PULSE_MIN_BRIGHTNESS + PULSE_BRIGHTNESS_RANGE * 
                 (fastSinF(t * PULSE_SPEED) * 0.5f + 0.5f);
  
  // Slight color shift during breath cycle (offset from breath)
  float colorShift = fastSinF(t * PULSE_SPEED + PULSE_PHASE_OFFSET) * 0.5f + 0.5f;
  
  uint8_t r, g, b;
  
  if (currentG > currentR && currentG > currentB) {
    // Green state - shift toward cyan at peak
    r = (uint8_t)(currentR * breath);
    g = (uint8_t)(currentG * breath);
    b = (uint8_t)((currentB + currentG * PULSE_GREEN_SHIFT * colorShift) * breath);
  } else if (currentR > currentG && currentR > currentB) {
    // Red state - shift toward orange at peak
    r = (uint8_t)(currentR * breath);
    g = (uint8_t)((currentG + currentR * PULSE_RED_SHIFT * colorShift) * breath);
    b = (uint8_t)(currentB * breath);
  } else {
    // Other states - just breathe
    r = (uint8_t)(currentR * breath);
    g = (uint8_t)(currentG * breath);
    b = (uint8_t)(currentB * breath);
  }

  fillAll(r, g, b);
  pixels.show();
}

#endif
