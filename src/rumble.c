#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include "joycon.h"

float clamp(float v, float min, float max) {
	if (v < min)
		return min;
	if (v > max)
		return max;
	return v;
}

void set_rdata(char *byte, uint16_t hf, uint8_t lf, uint8_t hf_amp, uint16_t lf_amp) {
	byte[0] = hf & 0xFF;
	byte[1] = hf_amp + ((hf >> 8) & 0xFF); //Add amp + 1st byte of frequency to amplitude byte
	byte[2] = lf + ((lf_amp >> 8) & 0xFF); //Add freq + 1st byte of LF amplitude to the frequency byte
	byte[3] = lf_amp & 0xFF;
}

unsigned char fha_to_u8(float a) {
	if (a > 1.0f)
		a = 1.0f;
	if (a < 0.0f)
		a = 0.0f;
	unsigned char v = ((float)0xc8) * a;
	// More than 0xc8 is unsafe for HA
	return v;
}

unsigned short fla_to_u16(float a) {
	// technically a could be a bit bigger for the LA value
	unsigned char v = fha_to_u8(a);
	unsigned short r = v >> 1;
	r |= (v & 1) << 15;
	return r;
}

void play_freq(joycon_t *jc,
			   float fhf, float flf,
			   float fha, float fla,
			   int duration) {
	flf = clamp(flf, 40.875885f, 626.286133f);
	fhf = clamp(fhf, 81.75177f, 1252.572266f);

	uint16_t hf = ((uint8_t)round(log2((double)fhf * 0.01) * 32.0) - 0x60) * 4;
	uint8_t lf = (uint8_t)round(log2((double)flf * 0.01) * 32.0) - 0x40;
	char buf[9] = {0};
	buf[0] = 0;
	set_rdata(buf + 1, hf, lf, fha_to_u8(fha), fla_to_u16(fla));
	set_rdata(buf + 5, hf, lf, fha_to_u8(fha), fla_to_u16(fla));
	memcpy(jc->rumble_data, buf + 1, 8);
	jc_set_vibration(jc, 1);
	jc_send_rumble(jc, buf);
	usleep(duration);
	buf[0] = 1;
	set_rdata(buf + 1, 0x0400, 0x01, 0, 0x8040);
	set_rdata(buf + 5, 0x0400, 0x01, 0, 0x8040);
	jc_set_vibration(jc, 0);
	memcpy(jc->rumble_data, buf + 1, 8);
	jc_send_rumble(jc, buf);
}
