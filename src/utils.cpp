#include "utils.h"
#include <stdio.h>
#include <stdint.h>

void dev_error(hid_device *d, const char *str) {
	const void *ptr = hid_error(d);
	if (!ptr)
		return;
	printf("%s", str);
	puts((char*)ptr);
	puts("");
}

void dump_buf(uint8_t *o) {
	int n = 0;
	for (int i = 0; i < 64; ++i) {
		if (n++ == 8) {
			n = 1;
			puts("");
		}
		printf("%02x ", o[i]);
	}
	puts("\n");
}

void decodeUint12(uint8_t *b, int *o) {
	o[0] = ((uint16_t)b[0]) | ((uint16_t)(b[1] & 0xF) << 8);
	o[1] = (((uint16_t)b[1]) >> 4) | (((uint16_t)b[2]) << 4);
}
