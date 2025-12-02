#ifndef EFFECT_SOLID_H
#define EFFECT_SOLID_H

#include "effects_base.h"

// Effect 1: Solid - Static color fill
void effectSolid() {
  fillAll(currentR, currentG, currentB);
  pixels.show();
}

#endif
