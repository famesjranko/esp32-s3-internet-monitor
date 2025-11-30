#ifndef EFFECTS_H
#define EFFECTS_H

// ===========================================
// EFFECTS SYSTEM
// ===========================================
// Each effect is in its own file for easy editing and extension.
// To add a new effect:
//   1. Create effects/effect_yourname.h with void effectYourname() function
//   2. Include it below
//   3. Add to Effect enum in InternetMonitor.ino
//   4. Add name to effectNames[] in InternetMonitor.ino
//   5. Add case to applyEffect() switch below
//   6. Add defaults to effectDefaults[] array

// Base utilities (must be included first)
#include "effects/effects_base.h"

// ===========================================
// INDIVIDUAL EFFECTS
// ===========================================

// Basic effects (0-4)
#include "effects/effect_off.h"
#include "effects/effect_solid.h"
#include "effects/effect_ripple.h"
#include "effects/effect_rainbow.h"
#include "effects/effect_rain.h"

// Visual effects (5-9)
#include "effects/effect_matrix.h"
#include "effects/effect_fire.h"
#include "effects/effect_plasma.h"
#include "effects/effect_ocean.h"
#include "effects/effect_nebula.h"

// Animated effects (10-17)
#include "effects/effect_life.h"
#include "effects/effect_pong.h"
#include "effects/effect_metaballs.h"
#include "effects/effect_interference.h"
#include "effects/effect_noise.h"
#include "effects/effect_ripple_pool.h"
#include "effects/effect_rings.h"
#include "effects/effect_ball.h"

// ===========================================
// EFFECT DISPATCHER
// ===========================================

void applyEffect() {
  switch (currentEffect) {
    // Basic effects
    case 0:  effectOff(); break;
    case 1:  effectSolid(); break;
    case 2:  effectRipple(); break;
    case 3:  effectRainbow(); break;
    case 4:  effectRain(); break;
    
    // Visual effects
    case 5:  effectMatrix(); break;
    case 6:  effectFire(); break;
    case 7:  effectPlasma(); break;
    case 8:  effectOcean(); break;
    case 9:  effectNebula(); break;
    
    // Animated effects
    case 10: effectLife(); break;
    case 11: effectPong(); break;
    case 12: effectMetaballs(); break;
    case 13: effectInterference(); break;
    case 14: effectNoise(); break;
    case 15: effectRipplePool(); break;
    case 16: effectRings(); break;
    case 17: effectBall(); break;
    
    default: effectSolid(); break;
  }
}

#endif // EFFECTS_H
