#ifndef RUMBLE_H
#define RUMBLE_H

#include <stdint.h>

struct joycon;

void play_freq(joycon *jc, float fhf, float flf, float fha, float fla, int duration);
void make_rumble_data(float fhf, float flf, float fha, float fla, uint8_t buf[8]);

#endif /* RUMBLE_H */