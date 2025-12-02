#ifndef EFFECT_RAINBOW_H
#define EFFECT_RAINBOW_H

#include "effects_base.h"

// Effect 3: Rainbow - Flowing rainbow (full color when online, tinted when offline)
void effectRainbow() {
  static unsigned long offset = 0;
  offset += 256 * (effectSpeed / 50.0);
  
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      int i = getPixelIndex(row, col);
      uint16_t hue = (offset + (row + col) * 4096) & 0xFFFF;
      uint32_t color = pixels.ColorHSV(hue, 255, 200);
      
      uint8_t r = (color >> 16) & 0xFF;
      uint8_t g = (color >> 8) & 0xFF;
      uint8_t b = color & 0xFF;
      
      if (!isInternetOK) {
        // Tint with current state color
        r = (r + currentR * 2) / 3;
        g = (g + currentG * 2) / 3;
        b = (b + currentB * 2) / 3;
      }
      
      pixels.setPixelColor(i, pixels.Color(r, g, b));
    }
  }
  pixels.show();
}

#endif
