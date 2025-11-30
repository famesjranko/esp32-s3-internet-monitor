#ifndef EFFECT_OFF_H
#define EFFECT_OFF_H

#include "effects_base.h"

// Effect 0: Off - All LEDs off
void effectOff() {
  pixels.clear();
  pixels.show();
}

#endif
