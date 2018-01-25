#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <hidapi/hidapi.h>

void dev_error(hid_device *d, const char *str);
void dump_buf(uint8_t *o);
void decodeUint12(uint8_t *b, int *o);

#endif /* UTILS_H */