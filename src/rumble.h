#ifndef RUMBLE_H
#define RUMBLE_H

#include "joycon.h"

void play_freq(joycon_t *jc, float fhf, float flf, float fha, float fla, int duration);
void make_rumble_data(float fhf, float flf, float fha, float fla, uint8_t buf[8]);

#endif /* RUMBLE_H */