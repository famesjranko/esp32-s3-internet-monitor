#ifndef EFFECTS_H
#define EFFECTS_H

/**
 * @file effects.h
 * @brief LED Effects System
 * 
 * Each effect is in its own file for easy editing and extension.
 * 
 * To add a new effect:
 *   1. Create effects/effect_yourname.h with void effectYourname() function
 *   2. Add reset function: void resetYournameEffect()
 *   3. Include it below
 *   4. Add to Effect enum in core/types.h
 *   5. Add name to effectNames[] in InternetMonitor.ino
 *   6. Add case to applyEffect() switch below
 *   7. Add defaults to effectDefaults[] array
 *   8. Add reset call to resetAllEffectState() in effects_base.h
 */

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

/**
 * Apply the currently selected LED effect
 * Called from LED task at 60fps
 * Uses currentEffect global to determine which effect to run
 */
void applyEffect() {
  switch (currentEffect) {
    // Basic effects
    case EFFECT_OFF:         effectOff(); break;
    case EFFECT_SOLID:       effectSolid(); break;
    case EFFECT_RIPPLE:      effectRipple(); break;
    case EFFECT_RAINBOW:     effectRainbow(); break;
    case EFFECT_RAIN:        effectRain(); break;
    
    // Visual effects
    case EFFECT_MATRIX:      effectMatrix(); break;
    case EFFECT_FIRE:        effectFire(); break;
    case EFFECT_PLASMA:      effectPlasma(); break;
    case EFFECT_OCEAN:       effectOcean(); break;
    case EFFECT_NEBULA:      effectNebula(); break;
    
    // Animated effects
    case EFFECT_LIFE:        effectLife(); break;
    case EFFECT_PONG:        effectPong(); break;
    case EFFECT_METABALLS:   effectMetaballs(); break;
    case EFFECT_INTERFERENCE:effectInterference(); break;
    case EFFECT_NOISE:       effectNoise(); break;
    case EFFECT_RIPPLE_POOL: effectRipplePool(); break;
    case EFFECT_RINGS:       effectRings(); break;
    case EFFECT_BALL:        effectBall(); break;
    
    default: effectSolid(); break;
  }
}

#endif // EFFECTS_H
