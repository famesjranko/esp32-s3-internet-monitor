#ifndef EFFECT_LIFE_H
#define EFFECT_LIFE_H

#include "effects_base.h"

// Cycle detection - store hashes of last N states
#define LIFE_HISTORY_SIZE    16  // Detect cycles up to 16 generations
#define LIFE_MUTATION_CHANCE 2   // Percent chance per generation to mutate
#define LIFE_NUM_TRIBES      2   // Two families

// Life state (static, persists between frames)
// Grid stores: 0 = dead, 1 = tribe A, 2 = tribe B
static uint8_t lifeGrid[64];
static uint8_t lifeNextGrid[64];
static uint32_t lifeHistory[LIFE_HISTORY_SIZE];
static int lifeHistoryIndex = 0;
static unsigned long lifeLastUpdate = 0;
static int lifeGeneration = 0;
static bool lifeInitialized = false;

// Get tribe colors based on connection state
static void getTribeColors(uint8_t tribe, uint8_t* r, uint8_t* g, uint8_t* b) {
  if (currentG > currentR && currentG > currentB) {
    // UP state (green) - cyan and green tribes
    if (tribe == 1) {
      *r = 0; *g = 180; *b = 180;  // Cyan
    } else {
      *r = 0; *g = 200; *b = 0;    // Green
    }
  } else if (currentR > 0 && currentG > 0 && currentB < 50) {
    // DEGRADED state (yellow/orange) - yellow and orange tribes
    if (tribe == 1) {
      *r = 200; *g = 180; *b = 0;  // Yellow
    } else {
      *r = 220; *g = 100; *b = 0;  // Orange
    }
  } else {
    // DOWN state (red) - red and dark red tribes
    if (tribe == 1) {
      *r = 200; *g = 0; *b = 0;    // Red
    } else {
      *r = 150; *g = 0; *b = 30;   // Dark red/maroon
    }
  }
}

// Simple hash of grid state (FNV-1a inspired) - only checks alive/dead
static uint32_t hashGrid(uint8_t* grid) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < 64; i++) {
    hash ^= (grid[i] > 0) ? 1 : 0;
    hash *= 16777619u;
  }
  return hash;
}

// Check if hash exists in history (cycle detected)
static bool isCycleDetected(uint32_t hash) {
  for (int i = 0; i < LIFE_HISTORY_SIZE; i++) {
    if (lifeHistory[i] == hash) return true;
  }
  return false;
}

// Reset function - call when switching to this effect
void resetLifeEffect() {
  lifeInitialized = false;
  lifeGeneration = 0;
  lifeHistoryIndex = 0;
  lifeLastUpdate = 0;
  for (int i = 0; i < LIFE_HISTORY_SIZE; i++) {
    lifeHistory[i] = 0;
  }
}

// Effect 11: Game of Life - Conway's with tribe/family coloring
void effectLife() {
  // Initialize or reseed
  if (!lifeInitialized) {
    for (int i = 0; i < 64; i++) {
      if (random(100) < 40) {
        lifeGrid[i] = 1 + random(LIFE_NUM_TRIBES);  // Random tribe 1-2
      } else {
        lifeGrid[i] = 0;
      }
    }
    lifeGeneration = 0;
    lifeHistoryIndex = 0;
    for (int i = 0; i < LIFE_HISTORY_SIZE; i++) {
      lifeHistory[i] = 0;
    }
    lifeInitialized = true;
  }
  
  unsigned long now = millis();
  float speedMult = getTimeScale();
  
  if (now - lifeLastUpdate > (250 / speedMult)) {
    lifeLastUpdate = now;
    lifeGeneration++;
    
    // Calculate next generation
    for (int row = 0; row < MATRIX_SIZE; row++) {
      for (int col = 0; col < MATRIX_SIZE; col++) {
        int neighbors = 0;
        uint8_t tribeCounts[LIFE_NUM_TRIBES] = {0, 0};
        
        // Count neighbors and their tribes (with wraparound)
        for (int dr = -1; dr <= 1; dr++) {
          for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            int nr = (row + dr + MATRIX_SIZE) % MATRIX_SIZE;
            int nc = (col + dc + MATRIX_SIZE) % MATRIX_SIZE;
            uint8_t neighborTribe = lifeGrid[nr * MATRIX_SIZE + nc];
            if (neighborTribe > 0) {
              neighbors++;
              tribeCounts[neighborTribe - 1]++;
            }
          }
        }
        
        int idx = row * MATRIX_SIZE + col;
        uint8_t currentTribe = lifeGrid[idx];
        
        // Conway's rules with tribe inheritance
        if (currentTribe > 0) {
          // Cell is alive
          if (neighbors == 2 || neighbors == 3) {
            // Survives - keep same tribe
            lifeNextGrid[idx] = currentTribe;
          } else {
            // Dies
            lifeNextGrid[idx] = 0;
          }
        } else {
          // Cell is dead
          if (neighbors == 3) {
            // Born - inherit majority tribe from parents
            if (tribeCounts[0] > tribeCounts[1]) {
              lifeNextGrid[idx] = 1;
            } else if (tribeCounts[1] > tribeCounts[0]) {
              lifeNextGrid[idx] = 2;
            } else {
              // Tied - random
              lifeNextGrid[idx] = 1 + random(2);
            }
          } else {
            lifeNextGrid[idx] = 0;
          }
        }
      }
    }
    
    // Random mutation - occasionally flip a random cell
    if (random(100) < LIFE_MUTATION_CHANCE) {
      int mutateIdx = random(64);
      if (lifeNextGrid[mutateIdx] > 0) {
        lifeNextGrid[mutateIdx] = 0;  // Kill a cell
      } else {
        lifeNextGrid[mutateIdx] = 1 + random(LIFE_NUM_TRIBES);  // Spawn random tribe
      }
    }
    
    // Check each 4x4 quadrant for emptiness - maybe spawn a small pattern
    // Only 20% chance per empty quadrant - gives gliders time to cross
    for (int qr = 0; qr < 2; qr++) {
      for (int qc = 0; qc < 2; qc++) {
        int startRow = qr * 4;
        int startCol = qc * 4;
        bool empty = true;
        
        for (int r = 0; r < 4 && empty; r++) {
          for (int c = 0; c < 4 && empty; c++) {
            if (lifeNextGrid[(startRow + r) * MATRIX_SIZE + (startCol + c)] > 0) {
              empty = false;
            }
          }
        }
        
        // Only 20% chance to spawn in empty quadrant
        if (empty && random(100) < 20) {
          // Spawn a small viable pattern (not just one cell)
          // Pick a random pattern type and position within quadrant
          uint8_t tribe = 1 + random(LIFE_NUM_TRIBES);
          int patternType = random(4);
          int baseR = startRow + 1;  // Offset to fit pattern
          int baseC = startCol + 1;
          
          switch (patternType) {
            case 0:  // Horizontal line (blinker)
              lifeNextGrid[baseR * MATRIX_SIZE + baseC] = tribe;
              lifeNextGrid[baseR * MATRIX_SIZE + baseC + 1] = tribe;
              lifeNextGrid[baseR * MATRIX_SIZE + baseC + 2] = tribe;
              break;
            case 1:  // Vertical line (blinker)
              lifeNextGrid[baseR * MATRIX_SIZE + baseC] = tribe;
              lifeNextGrid[(baseR + 1) * MATRIX_SIZE + baseC] = tribe;
              lifeNextGrid[(baseR + 2) * MATRIX_SIZE + baseC] = tribe;
              break;
            case 2:  // L-shape
              lifeNextGrid[baseR * MATRIX_SIZE + baseC] = tribe;
              lifeNextGrid[(baseR + 1) * MATRIX_SIZE + baseC] = tribe;
              lifeNextGrid[(baseR + 1) * MATRIX_SIZE + baseC + 1] = tribe;
              break;
            case 3:  // Block (stable but takes space)
              lifeNextGrid[baseR * MATRIX_SIZE + baseC] = tribe;
              lifeNextGrid[baseR * MATRIX_SIZE + baseC + 1] = tribe;
              lifeNextGrid[(baseR + 1) * MATRIX_SIZE + baseC] = tribe;
              lifeNextGrid[(baseR + 1) * MATRIX_SIZE + baseC + 1] = tribe;
              break;
          }
        }
      }
    }
    
    // Tribe balancing - count tribes and boost minority aggressively
    int tribeACount = 0, tribeBCount = 0;
    for (int i = 0; i < 64; i++) {
      if (lifeNextGrid[i] == 1) tribeACount++;
      else if (lifeNextGrid[i] == 2) tribeBCount++;
    }
    
    int total = tribeACount + tribeBCount;
    
    // If one tribe has more than 70% of cells, flip some to balance
    if (total > 0) {
      int targetFlips = 0;
      uint8_t fromTribe = 0, toTribe = 0;
      
      if (tribeACount > total * 7 / 10) {
        // Tribe A dominates - flip some to B
        targetFlips = (tribeACount - total / 2) / 3;  // Flip ~1/3 of excess
        fromTribe = 1;
        toTribe = 2;
      } else if (tribeBCount > total * 7 / 10) {
        // Tribe B dominates - flip some to A
        targetFlips = (tribeBCount - total / 2) / 3;
        fromTribe = 2;
        toTribe = 1;
      }
      
      // Apply flips randomly
      for (int f = 0; f < targetFlips; f++) {
        for (int attempts = 0; attempts < 20; attempts++) {
          int idx = random(64);
          if (lifeNextGrid[idx] == fromTribe) {
            lifeNextGrid[idx] = toTribe;
            break;
          }
        }
      }
    }
    
    // If one tribe is completely gone, spawn some
    if (tribeACount == 0 && total > 0) {
      // Spawn tribe A somewhere
      for (int i = 0; i < 3; i++) {
        int idx = random(64);
        if (lifeNextGrid[idx] == 2) lifeNextGrid[idx] = 1;
      }
    } else if (tribeBCount == 0 && total > 0) {
      // Spawn tribe B somewhere  
      for (int i = 0; i < 3; i++) {
        int idx = random(64);
        if (lifeNextGrid[idx] == 1) lifeNextGrid[idx] = 2;
      }
    }
    
    // Check for cycles using hash
    uint32_t nextHash = hashGrid(lifeNextGrid);
    if (isCycleDetected(nextHash)) {
      lifeInitialized = false;
    } else {
      lifeHistory[lifeHistoryIndex] = nextHash;
      lifeHistoryIndex = (lifeHistoryIndex + 1) % LIFE_HISTORY_SIZE;
      memcpy(lifeGrid, lifeNextGrid, 64);
    }
  }
  
  // Render with tribe colors
  for (int row = 0; row < MATRIX_SIZE; row++) {
    for (int col = 0; col < MATRIX_SIZE; col++) {
      int idx = row * MATRIX_SIZE + col;
      uint8_t tribe = lifeGrid[idx];
      
      if (tribe > 0) {
        uint8_t r, g, b;
        getTribeColors(tribe, &r, &g, &b);
        setPixelAt(row, col, r, g, b);
      } else {
        // Dead - dim background based on state
        setPixelAt(row, col, currentR / 15, currentG / 15, currentB / 15);
      }
    }
  }
  pixels.show();
}

#endif
