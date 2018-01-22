#ifndef RUMBLE_H
#define RUMBLE_H

#include "joycon.h"

void play_freq(joycon_t *jc, float fhf, float flf, float fha, float fla, int duration);

#endif /* RUMBLE_H */