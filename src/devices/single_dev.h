#ifndef SINGLE_DEV_H
#define SINGLE_DEV_H

#include <joycon.h>

typedef struct {
	jc_dev_interface_t base;
	joycon_t *jc;
	int jc_revents;
	int ui_revents;
} jc_device_t;

jc_dev_interface_t *jd_device_from_jc(joycon_t *jc);

#endif /* SINGLE_DEV_H */