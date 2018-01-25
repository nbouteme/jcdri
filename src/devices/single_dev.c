#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <rumble.h>

#include "common_dev.h"
#include "single_dev.h"

void play_effect_single(jc_dev_interface_t *dev, int id) {
	jc_device_t *self = (void*)dev;
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
		memcpy(self->jc->rumble_data, rb + 1, 8);
		jc_set_vibration(self->jc, 1);
		break;
	}
}

void stop_effect_single(jc_dev_interface_t *dev, int id) {
	jc_device_t *self = (void*)dev;
	(void)id;
	jc_set_vibration(self->jc, 0);
}

int read_fds_single(jc_dev_interface_t *dev, struct pollfd *pfd) {
	jc_device_t *self = (void*)dev;

	self->ui_revents = pfd[0].revents;
	self->jc_revents = pfd[1].revents;
	return 2;
}

int write_fds_single(jc_dev_interface_t *dev, struct pollfd *pfd) {
	jc_device_t *self = (void*)dev;
	// Linux hidraw device and uinput never replies with POLLOUT
	
	*pfd++ = (struct pollfd) {
		dev->uifd, POLLIN, 0
	};

	*pfd++ = (struct pollfd) {
		self->jc->dev->device_handle, POLLIN, 0
	};

	return 2;
}

int jd_post_events_single(jc_device_t *dev) {
	struct libevdev_uinput *uidev = dev->base.uidev;
	joycon_t *jc = dev->jc;

	assert_null(libevdev_uinput_write_event(uidev, EV_ABS, ABS_X, jc->rstick[0]));
	assert_null(libevdev_uinput_write_event(uidev, EV_ABS, ABS_Y, jc->rstick[1]));

	if (jc->imu_enabled) {
		assert_null(libevdev_uinput_write_event(uidev, EV_ABS, ABS_MISC, jc->acc_comp.x));
		assert_null(libevdev_uinput_write_event(uidev, EV_ABS, ABS_MISC + 1, jc->acc_comp.y));
		assert_null(libevdev_uinput_write_event(uidev, EV_ABS, ABS_MISC + 2, jc->acc_comp.z));
		assert_null(libevdev_uinput_write_event(uidev, EV_ABS, ABS_RX, jc->gyro_comp.x));
		assert_null(libevdev_uinput_write_event(uidev, EV_ABS, ABS_RY, jc->gyro_comp.y));
		assert_null(libevdev_uinput_write_event(uidev, EV_ABS, ABS_RZ, jc->gyro_comp.z));
	}

	if (jc->type == LEFT) {
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_A, jc->left));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_B, jc->down));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_X, jc->right));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_Y, jc->up));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TL, jc->lsl));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TR, jc->lsr));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TL2, jc->l));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TR2, jc->zl));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_JOYSTICK, jc->ls));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_START, jc->minus));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_SELECT, jc->capture));
	} else {
		/* TODO: Maybe wrong */
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_A, jc->a));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_B, jc->b));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_X, jc->x));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_Y, jc->y));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TL, jc->rsl));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TR, jc->rsr));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TL2, jc->r));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TR2, jc->zr));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_JOYSTICK, jc->rs));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_START, jc->plus));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_SELECT, jc->home));
	}
	
	assert_null(libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0));

	return 1;
}

void run_events_single(jc_dev_interface_t *dev) {
	jc_device_t *self = (void*)dev;
	if (self->jc_revents & POLLIN) {
		jc_poll(self->jc);
		jd_post_events_single(self);
	}
	if (self->ui_revents & POLLIN) {
		handle_input_events(dev);
	}
}

jc_dev_interface_t *jd_device_from_jc(joycon_t *jc) {
	jc_device_t *ret;
	char name[] = "EJoycon L";
	struct libevdev *dev = 	libevdev_new();
	struct input_absinfo ain[2] = { {
			jc->rstick[0],
			jc->lcal.min_x, jc->lcal.max_x,
			5, 10, 1
		}, {
			jc->rstick[1],
			jc->lcal.min_y, jc->lcal.max_y,
			5, 10, 1
		} };

	struct input_absinfo abs =  {
		jc->acc_comp.x,
		-32768, 32767,
		0, 0, 1
	};
	if (jc->type != LEFT) {
		name[8] = 'R';
		ain[0].minimum = jc->rcal.min_x;
		ain[0].maximum = jc->rcal.max_x;
		ain[1].minimum = jc->rcal.min_y;
		ain[1].maximum = jc->rcal.max_y;
	}

	libevdev_set_name(dev, name);
	libevdev_set_id_vendor(dev, 0x5652);
	libevdev_set_id_product(dev, 0x4432 + (jc->type != LEFT));

	assert_null(libevdev_enable_property(dev, INPUT_PROP_BUTTONPAD));

	assert_null(libevdev_enable_event_type(dev, EV_FF));
	assert_null(libevdev_enable_event_code(dev, EV_FF, FF_RUMBLE, NULL));

	assert_null(libevdev_enable_event_type(dev, EV_ABS));
	// 2 Axes pour 1 Stick
	assert_null(libevdev_enable_event_code(dev, EV_ABS, ABS_X, &ain[0]));
	assert_null(libevdev_enable_event_code(dev, EV_ABS, ABS_Y, &ain[1]));

	// 3 Axes acceleromètre + 3 Axes Gyroscope
	abs.value = jc->acc_comp.x;
	assert_null(libevdev_enable_event_code(dev, EV_ABS, ABS_MISC, &abs));
	abs.value = jc->acc_comp.y;
	assert_null(libevdev_enable_event_code(dev, EV_ABS, ABS_MISC + 1, &abs));
	abs.value = jc->acc_comp.z;
	assert_null(libevdev_enable_event_code(dev, EV_ABS, ABS_MISC + 2, &abs));
	abs.value = jc->gyro_comp.x;
	assert_null(libevdev_enable_event_code(dev, EV_ABS, ABS_RX, &abs));
	abs.value = jc->gyro_comp.y;
	assert_null(libevdev_enable_event_code(dev, EV_ABS, ABS_RY, &abs));
	abs.value = jc->gyro_comp.z;
	assert_null(libevdev_enable_event_code(dev, EV_ABS, ABS_RZ, &abs));

	assert_null(libevdev_enable_event_type(dev, EV_KEY));
	// 11 boutons par manette
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_A, NULL));
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_B, NULL));
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_X, NULL));
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_Y, NULL));
	// SL/SR
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_TL, NULL));
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_TR, NULL));
	// L/ZL || R/ZR
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_TL2, NULL));
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_TR2, NULL));
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_JOYSTICK, NULL));
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_START, NULL));
	assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_SELECT, NULL));

	ret = calloc(1, sizeof(*ret));
	assert_null(libevdev_uinput_create_from_device(dev, LIBEVDEV_UINPUT_OPEN_MANAGED, &ret->base.uidev));
	ret->base.dev = dev;
	ret->base.uifd = libevdev_uinput_get_fd(ret->base.uidev);
	ret->base.play_effect = play_effect_single;
	ret->base.stop_effect = stop_effect_single;
	ret->base.write_fds = write_fds_single;
	ret->base.read_fds = read_fds_single;
	ret->base.run_events = run_events_single;
	ret->jc = jc;
	return &ret->base;
}
