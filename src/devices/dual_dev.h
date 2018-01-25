#ifndef DUAL_DEV_H
#define DUAL_DEV_H

#include <joycon.h>
#include "common_dev.h"


typedef struct {
	jc_dev_interface_t base;
	joycon_t *left;
	joycon_t *right;
	int left_revents;
	int right_revents;
	int ui_revents;
} djc_device_t;

jc_dev_interface_t *jd_device_from_jc2(joycon_t *left, joycon_t *right);

#endif /* DUAL_DEV_H */