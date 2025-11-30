#ifndef EFFECT_BALL_H
#define EFFECT_BALL_H

#include "effects_base.h"

// Ball physics parameters
#define BALL_INITIAL_VX       0.12f
#define BALL_INITIAL_VY       0.08f
#define BALL_UPDATE_DELAY_MS  20

// Glow parameters
#define BALL_GLOW_RADIUS      4.0f

// Ball state (static, persists between frames)
static float ballX = 4.0f, ballY = 4.0f;
static float ballVX = BALL_INITIAL_VX, ballVY = BALL_INITIAL_VY;
static unsigned long ballLastUpdate = 0;

// Reset function - call when switching to this effect
void resetBallEffect() {
  ballX = 4.0f;
  ballY = 4.0f;
  ballVX = BALL_INITIAL_VX;
  ballVY = BALL_INITIAL_VY;
  ballLastUpdate = 0;
}

// Effect 18: Ball - Bouncing ball with glow
void effectBall() {
  unsigned long now = millis();
  float speedMult = getTimeScale();
  
  if (now - ballLastUpdate > (BALL_UPDATE_DELAY_MS / speedMult)) {
    ballLastUpdate = now;
    
    ballX += ballVX;
    ballY += ballVY;
    
    // Bounce off walls
    if (ballX <= 0 || ballX >= (MATRIX_SIZE - 1)) ballVX = -ballVX;
    if (ballY <= 0 || ballY >= (MATRIX_SIZE - 1)) ballVY = -ballVY;
    
    // Keep in bounds
    if (ballX < 0) ballX = 0;
    if (ballX > (MATRIX_SIZE - 1)) ballX = MATRIX_SIZE - 1;
    if (ballY < 0) ballY = 0;
    if (ballY > (MATRIX_SIZE - 1)) ballY = MATRIX_SIZE - 1;
  }
  
  // Render with glow
  for (int row = 0; row < MATRIX_SIZE; row++) {
    for (int col = 0; col < MATRIX_SIZE; col++) {
      // Distance from ball center (fast sqrt)
      float dist = fastDist(col - ballX, row - ballY);
      
      // Glow falloff
      float v = 1.0f - dist / BALL_GLOW_RADIUS;
      if (v < 0) v = 0;
      v = v * v;  // Sharper falloff (quadratic)
      
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
