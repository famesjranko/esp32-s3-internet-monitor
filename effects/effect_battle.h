#ifndef EFFECT_BATTLE_H
#define EFFECT_BATTLE_H

#include "effects_base.h"

// Battle visual parameters
#define BATTLE_UPDATE_MS  60

// Particle system - troops flowing toward battle line
#define MAX_PARTICLES     24  // 12 per side

struct BattleParticle {
  float x;        // Column position (float for smooth movement)
  int row;        // Row (0-7)
  uint8_t tribe;  // 1=cyan(left), 2=green(right)
  bool active;
};

static BattleParticle battleParticles[MAX_PARTICLES];
static float battleLine = 3.5f;  // Where armies clash (shifts slightly)
static unsigned long battleLastUpdate = 0;
static bool battleInitialized = false;

// Get tribe colors based on connection state
static void getBattleColors(uint8_t tribe, uint8_t* r, uint8_t* g, uint8_t* b) {
  if (currentG > currentR && currentG > currentB) {
    if (tribe == 1) { *r = 0; *g = 180; *b = 180; }      // Cyan
    else { *r = 0; *g = 200; *b = 0; }                    // Green
  } else if (currentR > 0 && currentG > 0 && currentB < 50) {
    if (tribe == 1) { *r = 200; *g = 180; *b = 0; }      // Yellow
    else { *r = 220; *g = 100; *b = 0; }                  // Orange
  } else {
    if (tribe == 1) { *r = 200; *g = 0; *b = 0; }        // Red
    else { *r = 150; *g = 0; *b = 30; }                   // Dark red
  }
}

// Spawn a new particle for a tribe
static void spawnParticle(uint8_t tribe) {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!battleParticles[i].active) {
      battleParticles[i].active = true;
      battleParticles[i].tribe = tribe;
      battleParticles[i].row = random(MATRIX_SIZE);
      battleParticles[i].x = (tribe == 1) ? 0.0f : 7.0f;  // Start at edge
      return;
    }
  }
}

// Count active particles for a tribe
static int countParticles(uint8_t tribe) {
  int count = 0;
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (battleParticles[i].active && battleParticles[i].tribe == tribe) count++;
  }
  return count;
}

// Reset function
void resetBattleEffect() {
  battleInitialized = false;
  battleLastUpdate = 0;
}

// Effect 19: Battle - Eternal clash
void effectBattle() {
  if (!battleInitialized) {
    // Clear particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
      battleParticles[i].active = false;
    }
    
    // Spawn initial waves
    for (int i = 0; i < 8; i++) {
      spawnParticle(1);  // Cyan from left
      spawnParticle(2);  // Green from right
    }
    
    battleLine = 3.5f;
    battleInitialized = true;
  }
  
  unsigned long now = millis();
  float speedMult = getTimeScale();
  
  if (now - battleLastUpdate > (BATTLE_UPDATE_MS / speedMult)) {
    battleLastUpdate = now;
    
    // Slowly drift battle line (creates push/pull effect)
    battleLine += (random(100) - 50) / 500.0f;  // Tiny random drift
    battleLine = constrain(battleLine, 2.5f, 4.5f);  // Keep near center
    
    // Update particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
      if (!battleParticles[i].active) continue;
      
      BattleParticle* p = &battleParticles[i];
      float speed = 0.15f + random(10) / 100.0f;  // Slight speed variation
      
      if (p->tribe == 1) {
        // Cyan moves right
        p->x += speed;
        // Die at battle line (with some randomness)
        if (p->x >= battleLine - 0.5f + random(100) / 100.0f) {
          p->active = false;
        }
      } else {
        // Green moves left
        p->x -= speed;
        // Die at battle line
        if (p->x <= battleLine + 0.5f - random(100) / 100.0f) {
          p->active = false;
        }
      }
      
      // Also random death near battle line (combat casualties)
      float distToLine = abs(p->x - battleLine);
      if (distToLine < 1.5f && random(100) < 15) {
        p->active = false;
      }
    }
    
    // Spawn reinforcements to maintain flow
    if (countParticles(1) < 10) spawnParticle(1);
    if (countParticles(2) < 10) spawnParticle(2);
    
    // Extra spawns occasionally for wave effect
    if (random(100) < 20) spawnParticle(1);
    if (random(100) < 20) spawnParticle(2);
  }
  
  // Clear grid
  uint8_t grid[64] = {0};
  
  // Place particles on grid
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!battleParticles[i].active) continue;
    BattleParticle* p = &battleParticles[i];
    
    int col = (int)(p->x + 0.5f);
    if (col < 0 || col >= MATRIX_SIZE) continue;
    
    int idx = p->row * MATRIX_SIZE + col;
    // If collision, both might die (battle!)
    if (grid[idx] != 0 && grid[idx] != p->tribe) {
      // Clash! Random winner or both die
      if (random(100) < 30) {
        grid[idx] = 0;  // Both die
      }
      // else: existing one stays (won)
    } else if (grid[idx] == 0) {
      grid[idx] = p->tribe;
    }
  }
  
  // Render with battle line glow
  for (int row = 0; row < MATRIX_SIZE; row++) {
    for (int col = 0; col < MATRIX_SIZE; col++) {
      int idx = row * MATRIX_SIZE + col;
      
      if (grid[idx] > 0) {
        uint8_t r, g, b;
        getBattleColors(grid[idx], &r, &g, &b);
        setPixelAt(row, col, r, g, b);
      } else {
        // Empty - show dim "territory" color and battle line glow
        float distToLine = abs((float)col - battleLine);
        
        if (distToLine < 1.0f) {
          // Battle line - flickering sparks
          if (random(100) < 40) {
            setPixelAt(row, col, 60 + random(40), 40 + random(30), 20);
          } else {
            setPixelAt(row, col, 15, 10, 5);
          }
        } else if (col < battleLine) {
          // Cyan territory - very dim cyan
          setPixelAt(row, col, 0, 8, 10);
        } else {
          // Green territory - very dim green
          setPixelAt(row, col, 0, 10, 0);
        }
      }
    }
  }
  pixels.show();
}

#endif
