#ifndef KEYBOARD_DUAL_H
#define KEYBOARD_DUAL_H

#include <joycon.h>

typedef struct {
	struct jc_dev_interface_s base;
	joycon_t *l;
	joycon_t *r;
	int l_revents;
	int r_revents;
	int ui_revents;
} jc_kbd_device_t;

jc_dev_interface_t *jd_keyboard_from_jc2(joycon_t *, joycon_t *);


#endif /* KEYBOARD_DUAL_H */