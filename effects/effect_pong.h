#ifndef EFFECT_PONG_H
#define EFFECT_PONG_H

#include "effects_base.h"

// Pong state (static, persists between frames)
static float pongBallX = 4, pongBallY = 4;
static float pongVelX = 0.15f, pongVelY = 0.1f;
static int pongPaddle1 = 3, pongPaddle2 = 3;
static unsigned long pongLastUpdate = 0;

// Reset function - call when switching to this effect
void resetPongEffect() {
  pongBallX = 4;
  pongBallY = 4;
  pongVelX = 0.15f;
  pongVelY = 0.1f;
  pongPaddle1 = 3;
  pongPaddle2 = 3;
  pongLastUpdate = 0;
}

// Effect 12: Pong - Auto-playing pong game
void effectPong() {
  unsigned long now = millis();
  float speedMult = getTimeScale();
  
  if (now - pongLastUpdate > (30 / speedMult)) {
    pongLastUpdate = now;
    
    // Move ball
    pongBallX += pongVelX;
    pongBallY += pongVelY;
    
    // Paddle AI
    if (pongVelX < 0 && pongBallX < 4) {
      if (pongPaddle1 < pongBallY - 0.5f) pongPaddle1++;
      else if (pongPaddle1 > pongBallY + 0.5f) pongPaddle1--;
    }
    if (pongVelX > 0 && pongBallX > 4) {
      if (pongPaddle2 < pongBallY - 0.5f) pongPaddle2++;
      else if (pongPaddle2 > pongBallY + 0.5f) pongPaddle2--;
    }
    
    // Clamp paddles
    if (pongPaddle1 < 1) pongPaddle1 = 1;
    if (pongPaddle1 > 6) pongPaddle1 = 6;
    if (pongPaddle2 < 1) pongPaddle2 = 1;
    if (pongPaddle2 > 6) pongPaddle2 = 6;
    
    // Ball collision with paddles
    if (pongBallX <= 1 && fabs(pongBallY - pongPaddle1) < 1.5f) {
      pongVelX = fabs(pongVelX);
      pongVelY += (pongBallY - pongPaddle1) * 0.1f;
    }
    if (pongBallX >= 6 && fabs(pongBallY - pongPaddle2) < 1.5f) {
      pongVelX = -fabs(pongVelX);
      pongVelY += (pongBallY - pongPaddle2) * 0.1f;
    }
    
    // Ball collision with top/bottom
    if (pongBallY <= 0 || pongBallY >= 7) pongVelY = -pongVelY;
    
    // Reset if ball goes off screen
    if (pongBallX < 0 || pongBallX > 7) {
      pongBallX = 4;
      pongBallY = 4;
      pongVelX = (random(2) ? 0.15f : -0.15f);
      pongVelY = (random(100) - 50) / 500.0f;
    }
    
    // Clamp velocity
    if (pongVelY < -0.2f) pongVelY = -0.2f;
    if (pongVelY > 0.2f) pongVelY = 0.2f;
  }
  
  // Render
  pixels.clear();
  
  // Center line FIRST (dim)
  for (int r = 0; r < MATRIX_SIZE; r += 2) {
    setPixelAt(r, 3, currentR/8, currentG/8, currentB/8);
    setPixelAt(r, 4, currentR/8, currentG/8, currentB/8);
  }
  
  // Ball SECOND (so paddles can overwrite)
  int ballPixelX = (int)pongBallX;
  int ballPixelY = (int)pongBallY;
  if (ballPixelX < 0) ballPixelX = 0; if (ballPixelX > 7) ballPixelX = 7;
  if (ballPixelY < 0) ballPixelY = 0; if (ballPixelY > 7) ballPixelY = 7;
  setPixelAt(ballPixelY, ballPixelX,
    clamp255(currentR + 150), clamp255(currentG + 150), clamp255(currentB + 150));
  
  // Paddles LAST (always on top)
  for (int dy = -1; dy <= 1; dy++) {
    int y1 = pongPaddle1 + dy;
    int y2 = pongPaddle2 + dy;
    if (y1 < 0) y1 = 0; if (y1 > 7) y1 = 7;
    if (y2 < 0) y2 = 0; if (y2 > 7) y2 = 7;
    setPixelAt(y1, 0, currentR, currentG, currentB);
    setPixelAt(y2, 7, currentR, currentG, currentB);
  }
  
  pixels.show();
}

#endif
