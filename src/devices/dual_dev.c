#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <joycon.h>
#include <rumble.h>
#include "dual_dev.h"

void play_effect_duo(jc_dev_interface_t *dev, int id) {
	djc_device_t *self = (void*)dev;
	struct ff_effect *eff = dev->rumble_patterns + id;
	switch(eff->type) {
	case FF_RUMBLE:;
		uint8_t rb[9] = {0};
		float str = eff->u.rumble.strong_magnitude;
		float wea = eff->u.rumble.weak_magnitude;

		rb[0] = next_tick();
		str /= 32768;
		wea /= 49152;
		make_rumble_data(320, 160, str + 0.4, wea + 0.4, rb + 1);
		memcpy(self->left->rumble_data, rb + 1, 8);
		memcpy(self->right->rumble_data, rb + 1, 8);
		jc_set_vibration(self->left, 1);
		jc_set_vibration(self->right, 1);
		break;
	}
}

void stop_effect_duo(jc_dev_interface_t *dev, int id) {
	djc_device_t *self = (void*)dev;
	(void)self;
	(void)id;
	jc_set_vibration(self->left, 0);
	jc_set_vibration(self->right, 0);
}

int jd_post_events_duo(djc_device_t *dev) {
	struct libevdev_uinput *uidev = dev->base.uidev;
	joycon_t *left = dev->left;
	joycon_t *right = dev->right;

	assert_null(libevdev_uinput_write_event(uidev, EV_ABS, 0, left->rstick[0]));
	assert_null(libevdev_uinput_write_event(uidev, EV_ABS, 1, -left->rstick[1]));
	assert_null(libevdev_uinput_write_event(uidev, EV_ABS, 2, right->rstick[0]));
	assert_null(libevdev_uinput_write_event(uidev, EV_ABS, 3, -right->rstick[1]));

	if (left->imu_enabled && right->imu_enabled) {
		assert_null(libevdev_uinput_write_event(uidev, EV_ABS,  4, left->acc_comp.x));
		assert_null(libevdev_uinput_write_event(uidev, EV_ABS,  5, left->acc_comp.y));
		assert_null(libevdev_uinput_write_event(uidev, EV_ABS,  6, left->acc_comp.z));
		assert_null(libevdev_uinput_write_event(uidev, EV_ABS,  7, right->acc_comp.x));
		assert_null(libevdev_uinput_write_event(uidev, EV_ABS,  8, right->acc_comp.y));
		assert_null(libevdev_uinput_write_event(uidev, EV_ABS,  9, right->acc_comp.z));
		assert_null(libevdev_uinput_write_event(uidev, EV_ABS, 10, left->gyro_comp.x));
		assert_null(libevdev_uinput_write_event(uidev, EV_ABS, 11, left->gyro_comp.y));
		assert_null(libevdev_uinput_write_event(uidev, EV_ABS, 12, left->gyro_comp.z));
		assert_null(libevdev_uinput_write_event(uidev, EV_ABS, 13, right->gyro_comp.x));
		assert_null(libevdev_uinput_write_event(uidev, EV_ABS, 14, right->gyro_comp.y));
		assert_null(libevdev_uinput_write_event(uidev, EV_ABS, 15, right->gyro_comp.z));
	}

	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_A, right->a));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_B, right->b));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_X, right->x));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_Y, right->y));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_0, left->up));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_1, left->down));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_2, left->left));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_3, left->right));

	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TL, left->l));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TR, right->r));

	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TL2, left->zl));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TR2, right->zr));

	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_THUMBL, left->ls));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_THUMBR, right->rs));

	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_START, right->plus));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_SELECT, left->minus));

	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TRIGGER_HAPPY + 0, left->capture));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TRIGGER_HAPPY + 1, right->home));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TRIGGER_HAPPY + 2, left->lsl));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TRIGGER_HAPPY + 3, left->lsr));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TRIGGER_HAPPY + 4, right->rsl));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TRIGGER_HAPPY + 5, right->rsr));

	assert_null(libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0));
	return 1;
}

void run_events_dual(jc_dev_interface_t *dev) {
	djc_device_t *self = (void*)dev;
	if (self->left_revents & POLLIN) {
		jc_poll(self->left);
	}
	if (self->right_revents & POLLIN) {
		jc_poll(self->right);
	}
	jd_post_events_duo(self);
	if (self->ui_revents & POLLIN) {
		handle_input_events(dev);
	}
}

int write_fds_dual(jc_dev_interface_t *dev, struct pollfd *pfd) {
	djc_device_t *self = (void*)dev;
	// Linux hidraw device and uinput never replies with POLLOUT
	
	*pfd++ = (struct pollfd) {
		dev->uifd, POLLIN, 0
	};

	*pfd++ = (struct pollfd) {
		self->left->dev->device_handle, POLLIN, 0
	};

	*pfd++ = (struct pollfd) {
		self->right->dev->device_handle, POLLIN, 0
	};

	return 3;
}

int read_fds_dual(jc_dev_interface_t *dev, struct pollfd *pfd) {
	djc_device_t *self = (void*)dev;

	self->ui_revents = pfd[0].revents;
	self->left_revents = pfd[1].revents;
	self->right_revents = pfd[2].revents;
	return 3;
}

jc_dev_interface_t *jd_device_from_jc2(joycon_t *left, joycon_t *right) {
	djc_device_t *ret;
	char name[] = "EJoycon Dual";
	struct libevdev *dev = 	libevdev_new();
	struct input_absinfo base = {
		0, -100, 100, 0, 0, 1
	};
	(void)base;
	struct input_absinfo ain[4] = { {
			left->rstick[0],
			left->lcal.min_x, left->lcal.max_x,
			left->stick_prop.deadzone, 10, 1
		}, {
			left->rstick[1],
			left->lcal.min_y, left->lcal.max_y,
			left->stick_prop.deadzone, 10, 1
		}, {
			right->rstick[0],
			right->rcal.min_x, right->rcal.max_x,
			right->stick_prop.deadzone, 10, 1
		}, {
			right->rstick[1],
			right->rcal.min_y, right->rcal.max_y,
			right->stick_prop.deadzone, 10, 1
		}};

	struct input_absinfo abs =  {
		left->acc_comp.x,
		SHRT_MIN, SHRT_MAX,
		0, 0, 1
	};
	(void)abs;

	libevdev_set_name(dev, name);
	libevdev_set_id_vendor(dev, 0x5652);
	libevdev_set_id_product(dev, 0x4444);

	(void)ain;
	assert_null(libevdev_enable_property(dev, INPUT_PROP_BUTTONPAD));

	assert_null(libevdev_enable_event_type(dev, EV_FF));
	assert_null(libevdev_enable_event_code(dev, EV_FF, FF_RUMBLE, NULL));

	assert_null(libevdev_enable_event_type(dev, EV_ABS));
	// 2 Axes pour 2 Sticks
	assert_null(libevdev_enable_event_code(dev, EV_ABS, 0, &ain[0]));
	assert_null(libevdev_enable_event_code(dev, EV_ABS, 1, &ain[1]));
	assert_null(libevdev_enable_event_code(dev, EV_ABS, 2, &ain[2]));
	assert_null(libevdev_enable_event_code(dev, EV_ABS, 3, &ain[3]));

	// 6 Axes acceleromètre + 6 Axes Gyroscope
	abs.value = left->acc_comp.x;
	assert_null(libevdev_enable_event_code(dev, EV_ABS, 4, &abs));
	abs.value = left->acc_comp.y;
	assert_null(libevdev_enable_event_code(dev, EV_ABS, 5, &abs));
	abs.value = left->acc_comp.z;
	assert_null(libevdev_enable_event_code(dev, EV_ABS, 6, &abs));
	abs.value = right->acc_comp.x;
	assert_null(libevdev_enable_event_code(dev, EV_ABS, 7, &abs));
	abs.value = right->acc_comp.y;
	assert_null(libevdev_enable_event_code(dev, EV_ABS, 8, &abs));
	abs.value = right->acc_comp.z;
	assert_null(libevdev_enable_event_code(dev, EV_ABS, 9, &abs));

	abs.value = left->gyro_comp.x;
	assert_null(libevdev_enable_event_code(dev, EV_ABS, 10, &abs));
	abs.value = left->gyro_comp.y;
	assert_null(libevdev_enable_event_code(dev, EV_ABS, 11, &abs));
	abs.value = left->gyro_comp.z;
	assert_null(libevdev_enable_event_code(dev, EV_ABS, 12, &abs));
	abs.value = right->gyro_comp.x;
	assert_null(libevdev_enable_event_code(dev, EV_ABS, 13, &abs));
	abs.value = right->gyro_comp.y;
	assert_null(libevdev_enable_event_code(dev, EV_ABS, 14, &abs));
	abs.value = right->gyro_comp.z;
	assert_null(libevdev_enable_event_code(dev, EV_ABS, 15, &abs));

	assert_null(libevdev_enable_event_type(dev, EV_KEY));

	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_A, NULL));
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_B, NULL));
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_X, NULL));
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_Y, NULL));

	// DPAD
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_0, NULL));
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_1, NULL));
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_2, NULL));
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_3, NULL));
	// L/R
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_TL, NULL));
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_TR, NULL));
	// ZL / ZR
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_TL2, NULL));
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_TR2, NULL));

	// Button 2 stick
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_THUMBL, NULL));
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_THUMBR, NULL));

	// Plus Minus
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_START, NULL));
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_SELECT, NULL));

	// Capture / Home
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_TRIGGER_HAPPY, NULL));
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_TRIGGER_HAPPY + 1, NULL));

	// SL/SR Gauche et Droit
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_TRIGGER_HAPPY + 2, NULL));
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_TRIGGER_HAPPY + 3, NULL));
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_TRIGGER_HAPPY + 4, NULL));
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_TRIGGER_HAPPY + 5, NULL));

	ret = calloc(1, sizeof(*ret));
	assert_null(libevdev_uinput_create_from_device(dev, LIBEVDEV_UINPUT_OPEN_MANAGED, &ret->base.uidev));
	ret->base.dev = dev;
	ret->base.uifd = libevdev_uinput_get_fd(ret->base.uidev);
	ret->base.play_effect = play_effect_duo;
	ret->base.stop_effect = stop_effect_duo;
	ret->base.write_fds = write_fds_dual;
	ret->base.read_fds = read_fds_dual;
	ret->base.run_events = run_events_dual;
	ret->left = left;
	ret->right = right;
	return &ret->base;
}
