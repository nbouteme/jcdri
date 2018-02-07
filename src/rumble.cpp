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

void set_rdata(uint8_t *byte, uint16_t hf, uint8_t lf, uint8_t hf_amp, uint16_t lf_amp) {
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

void make_rumble_data(float fhf, float flf,
					  float fha, float fla, uint8_t buf[8]) {
	flf = clamp(flf, 40.875885f, 626.286133f);
	fhf = clamp(fhf, 81.75177f, 1252.572266f);

	uint16_t hf = ((uint8_t)round(log2((double)fhf * 0.01) * 32.0) - 0x60) * 4;
	uint8_t lf = (uint8_t)round(log2((double)flf * 0.01) * 32.0) - 0x40;
	set_rdata(buf, hf, lf, fha_to_u8(fha), fla_to_u16(fla));
	set_rdata(buf + 4, hf, lf, fha_to_u8(fha), fla_to_u16(fla));
}

void play_freq(jcdri::joycon *jc,
			   float fhf, float flf,
			   float fha, float fla,
			   int duration) {
	flf = clamp(flf, 40.875885f, 626.286133f);
	fhf = clamp(fhf, 81.75177f, 1252.572266f);

	uint16_t hf = ((uint8_t)round(log2((double)fhf * 0.01) * 32.0) - 0x60) * 4;
	uint8_t lf = (uint8_t)round(log2((double)flf * 0.01) * 32.0) - 0x40;
	uint8_t buf[9] = {0};
	buf[0] = 0;
	set_rdata(buf + 1, hf, lf, fha_to_u8(fha), fla_to_u16(fla));
	set_rdata(buf + 5, hf, lf, fha_to_u8(fha), fla_to_u16(fla));
	memcpy(jc->rumble_data, buf + 1, 8);
	make_rumble_data(fhf, flf, fha, fla, jc->rumble_data);
	jc->set_vibration(1);
	jc->send_rumble(buf);
	usleep(duration);
	buf[0] = 1;
	set_rdata(buf + 1, 0x0400, 0x01, 0, 0x8040);
	set_rdata(buf + 5, 0x0400, 0x01, 0, 0x8040);
	jc->set_vibration(0);
	memcpy(jc->rumble_data, buf + 1, 8);
	jc->send_rumble(buf);
}
