#pragma once
#include <cstdint>
struct Adafruit_NeoPixel {
  void setBrightness(uint8_t) {}
  void setPixelColor(int, uint32_t) {}
  void setPixelColor(int, uint8_t, uint8_t, uint8_t) {}
  uint32_t Color(uint8_t r, uint8_t g, uint8_t b) { return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b; }
  void show() {}
  void clear() {}
};
