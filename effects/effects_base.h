#ifndef EFFECTS_BASE_H
#define EFFECTS_BASE_H

#include <Adafruit_NeoPixel.h>
#include "../config.h"

// ===========================================
// EFFECT CONFIGURATION DATA
// ===========================================

// Effect names for display in UI
static const char* effectNames[] = {
  "Off", "Solid", "Ripple", "Rainbow", "Rain",
  "Matrix", "Fire", "Plasma", "Ocean", "Nebula", "Life",
  "Pong", "Metaballs", "Interference", "Noise", "Pool", "Rings", "Ball"
};

// Per-effect default brightness and speed
// Format: {brightness, speed}
static const uint8_t effectDefaults[][2] = {
  {5, 50},    // 0: Off (doesn't matter)
  {5, 50},    // 1: Solid (speed doesn't matter)
  {10, 72},   // 2: Ripple
  {5, 72},    // 3: Rainbow
  {10, 36},   // 4: Rain
  {5, 50},    // 5: Matrix
  {5, 51},    // 6: Fire
  {5, 100},   // 7: Plasma
  {5, 58},    // 8: Ocean
  {5, 58},    // 9: Nebula
  {5, 25},    // 10: Life
  {5, 36},    // 11: Pong
  {5, 100},   // 12: Metaballs
  {5, 50},    // 13: Interference
  {5, 84},    // 14: Noise
  {5, 80},    // 15: Pool
  {10, 57},   // 16: Rings
  {25, 57},   // 17: Ball
};

// ===========================================
// EXTERNAL REFERENCES (defined in main .ino)
// ===========================================

extern Adafruit_NeoPixel pixels;
extern volatile uint8_t currentR, currentG, currentB;
extern volatile uint8_t targetR, targetG, targetB;
extern uint8_t fadeStartR, fadeStartG, fadeStartB;
extern unsigned long fadeStartTime;
extern volatile uint8_t currentRotation;
extern volatile uint8_t effectSpeed;
extern volatile int currentEffect;
extern volatile bool isInternetOK;

// ===========================================
// SIN/COS LOOKUP TABLE
// ===========================================
// Pre-calculated sine table for fast effect calculations
// Values scaled to -127 to 127 (int8_t range)
// Use: sinLUT[(angle * SIN_TABLE_SIZE / 360) & 0xFF]

extern int8_t sinLUT[SIN_TABLE_SIZE];
extern bool lutInitialized;

// Initialize lookup tables (call once at startup)
inline void initLookupTables() {
  if (lutInitialized) return;
  for (int i = 0; i < SIN_TABLE_SIZE; i++) {
    sinLUT[i] = (int8_t)(sin(i * 2.0 * PI / SIN_TABLE_SIZE) * 127.0);
  }
  lutInitialized = true;
}

// Fast sine using lookup table
// Input: 0-255 (maps to 0-360 degrees)
// Output: -127 to 127
inline int8_t fastSin8(uint8_t angle) {
  return sinLUT[angle];
}

// Fast cosine using lookup table (sin + 90 degrees)
inline int8_t fastCos8(uint8_t angle) {
  return sinLUT[(angle + 64) & 0xFF];  // 64 = 90 degrees in 256 scale
}

// Fast sine for floats - returns -1.0 to 1.0
// Input: any float (will wrap)
inline float fastSinF(float x) {
  // Convert to 0-255 range
  int idx = (int)(x * 40.74f) & 0xFF;  // 40.74 = 256 / (2*PI)
  return sinLUT[idx] / 127.0f;
}

inline float fastCosF(float x) {
  int idx = ((int)(x * 40.74f) + 64) & 0xFF;
  return sinLUT[idx] / 127.0f;
}

// ===========================================
// FAST INVERSE SQUARE ROOT (Quake III algorithm)
// ===========================================
// Famous fast approximation - about 4x faster than sqrt()
// Accurate to ~1% which is fine for visual effects

inline float fastInvSqrt(float x) {
  float xhalf = 0.5f * x;
  int i = *(int*)&x;
  i = FAST_SQRT_MAGIC - (i >> 1);
  x = *(float*)&i;
  x = x * (1.5f - xhalf * x * x);  // One Newton-Raphson iteration
  return x;
}

// Fast square root using inverse sqrt
inline float fastSqrt(float x) {
  if (x <= 0) return 0;
  return x * fastInvSqrt(x);
}

// Fast distance calculation (avoids sqrt when possible)
inline float fastDist(float dx, float dy) {
  return fastSqrt(dx * dx + dy * dy);
}

// ===========================================
// LED HELPER FUNCTIONS
// ===========================================

// Map logical row/col to physical pixel index based on rotation
// Coordinate system: row 0 = top, col 0 = left (before rotation)
inline int getPixelIndex(int row, int col) {
  int r = row, c = col;
  switch (currentRotation) {
    case ROTATION_90:   // 90° CW
      r = col;
      c = 7 - row;
      break;
    case ROTATION_180:  // 180°
      r = 7 - row;
      c = 7 - col;
      break;
    case ROTATION_270:  // 270° CW
      r = 7 - col;
      c = row;
      break;
    // ROTATION_0 (default): no transformation needed
  }
  return r * MATRIX_SIZE + c;
}

// ===========================================
// CONSISTENT PIXEL API
// ===========================================
// Always use these functions for setting pixels to ensure
// consistent behavior regardless of color order settings

// Set pixel by index with RGB values
inline void setPixelRGB(int index, uint8_t r, uint8_t g, uint8_t b) {
  pixels.setPixelColor(index, r, g, b);
}

// Set pixel at row/col with rotation applied
inline void setPixelAt(int row, int col, uint8_t r, uint8_t g, uint8_t b) {
  pixels.setPixelColor(getPixelIndex(row, col), r, g, b);
}

// Set all pixels to same color
inline void fillAll(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < NUM_LEDS; i++) {
    pixels.setPixelColor(i, r, g, b);
  }
}

// ===========================================
// COLOR/FADE MANAGEMENT
// ===========================================

inline void setTargetColor(uint8_t r, uint8_t g, uint8_t b) {
  if (targetR == r && targetG == g && targetB == b) return;
  
  fadeStartR = currentR;
  fadeStartG = currentG;
  fadeStartB = currentB;
  targetR = r;
  targetG = g;
  targetB = b;
  fadeStartTime = millis();
}

inline void updateFade() {
  unsigned long elapsed = millis() - fadeStartTime;
  
  if (elapsed >= FADE_DURATION) {
    currentR = targetR;
    currentG = targetG;
    currentB = targetB;
  } else {
    float progress = (float)elapsed / FADE_DURATION;
    // Ease in-out curve
    progress = progress < 0.5f ? 2.0f * progress * progress : 1.0f - pow(-2.0f * progress + 2.0f, 2) / 2.0f;
    
    currentR = fadeStartR + (int)(targetR - fadeStartR) * progress;
    currentG = fadeStartG + (int)(targetG - fadeStartG) * progress;
    currentB = fadeStartB + (int)(targetB - fadeStartB) * progress;
  }
}

inline void fillMatrixImmediate(uint8_t r, uint8_t g, uint8_t b) {
  currentR = targetR = r;
  currentG = targetG = g;
  currentB = targetB = b;
  fillAll(r, g, b);
  pixels.show();
}

// ===========================================
// HSV TO RGB CONVERSION
// ===========================================

inline void hsvToRgb(float h, float s, float v, uint8_t* r, uint8_t* g, uint8_t* b) {
  float c = v * s;
  float x = c * (1 - fabs(fmod(h / 60.0f, 2) - 1));
  float m = v - c;
  
  float rf, gf, bf;
  if (h < 60)       { rf = c; gf = x; bf = 0; }
  else if (h < 120) { rf = x; gf = c; bf = 0; }
  else if (h < 180) { rf = 0; gf = c; bf = x; }
  else if (h < 240) { rf = 0; gf = x; bf = c; }
  else if (h < 300) { rf = x; gf = 0; bf = c; }
  else              { rf = c; gf = 0; bf = x; }
  
  *r = (uint8_t)((rf + m) * 255);
  *g = (uint8_t)((gf + m) * 255);
  *b = (uint8_t)((bf + m) * 255);
}

// ===========================================
// UTILITY FUNCTIONS
// ===========================================

// Clamp value to 0-255 range (for color calculations)
inline uint8_t clamp255(int value) {
  if (value < 0) return 0;
  if (value > 255) return 255;
  return (uint8_t)value;
}

// Linear interpolation (named lerpf to avoid std::lerp conflict)
inline float lerpf(float a, float b, float t) {
  return a + (b - a) * t;
}

// Map value from one range to another
inline float mapFloat(float x, float inMin, float inMax, float outMin, float outMax) {
  return (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

// Get animation time factor based on effect speed
inline float getTimeScale() {
  return effectSpeed / ANIM_SPEED_DIVISOR;
}

// Get current time in seconds, scaled by effect speed
inline float getScaledTime() {
  return millis() / 1000.0f * getTimeScale();
}

// ===========================================
// EFFECT RESET SYSTEM
// ===========================================
// Effects with static state should implement a reset function.
// Call resetAllEffectState() when switching effects to start fresh.

// ===========================================
// FACTORY RESET EFFECT
// ===========================================

/**
 * @brief Show factory reset progress on LED matrix
 * 
 * Displays a clear countdown effect:
 * - Red border that fills inward as time progresses
 * - At 0%: empty
 * - At 100%: full red matrix
 * 
 * @param progress 0.0 to 1.0 (0 = just started, 1 = complete)
 */
inline void showFactoryResetProgress(float progress) {
  pixels.clear();
  
  // Calculate how many "rings" to fill (0-4 rings for 8x8 matrix)
  int rings = (int)(progress * 5);  // 0, 1, 2, 3, or 4 rings
  
  for (int row = 0; row < MATRIX_SIZE; row++) {
    for (int col = 0; col < MATRIX_SIZE; col++) {
      // Calculate which ring this pixel is in (0 = outer, 3 = center)
      int ringX = min(col, MATRIX_SIZE - 1 - col);
      int ringY = min(row, MATRIX_SIZE - 1 - row);
      int ring = min(ringX, ringY);
      
      // Light up if within the current ring count
      if (ring < rings) {
        // Solid red for completed rings
        pixels.setPixelColor(row * MATRIX_SIZE + col, pixels.Color(255, 0, 0));
      } else if (ring == rings) {
        // Pulsing red for current ring (progress within this ring)
        float ringProgress = (progress * 5) - rings;
        uint8_t brightness = (uint8_t)(ringProgress * 255);
        pixels.setPixelColor(row * MATRIX_SIZE + col, pixels.Color(brightness, 0, 0));
      }
    }
  }
  pixels.show();
}

// Forward declarations for reset functions (implemented in each effect file)
void resetBallEffect();
void resetLifeEffect();
void resetMatrixEffect();
void resetNoiseEffect();
void resetPongEffect();
void resetRainEffect();

// Master reset function - calls all effect resets
inline void resetAllEffectState() {
  resetBallEffect();
  resetLifeEffect();
  resetMatrixEffect();
  resetNoiseEffect();
  resetPongEffect();
  resetRainEffect();
}

#endif // EFFECTS_BASE_H
