#include "joycon.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <limits.h>

#include "rumble.h"
#include <sys/poll.h>
/*
#define assert_null(x) {												\
		int __error_code = x;											\
		if (__error_code) {												\
			char *s = strerror(-__error_code);							\
			fprintf(stderr, "Failed at %s:%d with code %d %s\n", __FILE__, __LINE__, __error_code, s); \
			return 0;													\
		}																\
	}																	\
*/
#define assert_null(x) x

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

void upload_effect(jc_dev_interface_t *dev, struct ff_effect effect) {
	int idx = effect.id;
	if ((dev->used_patterns >> idx) & 1)
		return;
	dev->rumble_patterns[idx] = effect;
	dev->used_patterns |= 1 << idx;
}

void erase_effect(jc_dev_interface_t *dev, int effect) {
	int idx = effect;
	if (!((dev->used_patterns >> idx) & 1))
		return;
	dev->used_patterns ^= 1 << idx;
}

void handle_input_events(jc_dev_interface_t *dev) {
	int fd = dev->uifd;
	struct input_event ev;
	read(fd, &ev, sizeof(ev));
	switch(ev.type) {
	case EV_FF:
		if (ev.code == FF_GAIN)
			return;
		if (ev.value)
			dev->play_effect(dev, ev.code);
		else
			dev->stop_effect(dev, ev.code);
		break;
	case EV_UINPUT:
		switch (ev.code) {
		case UI_FF_UPLOAD:;
			struct uinput_ff_upload upload;
			memset(&upload, 0, sizeof(upload));
			upload.request_id = ev.value;
			ioctl(fd, UI_BEGIN_FF_UPLOAD, &upload);
			upload_effect(dev, upload.effect);
			upload.retval = 0;
			ioctl(fd, UI_END_FF_UPLOAD, &upload);
			break;
		case UI_FF_ERASE:;
			struct uinput_ff_erase erase;
			memset(&erase, 0, sizeof(erase));
			erase.request_id = ev.value;
			ioctl(fd, UI_BEGIN_FF_ERASE, &erase);
			erase_effect(dev, erase.effect_id);
			erase.retval = 0;
			ioctl(fd, UI_END_FF_ERASE, &erase);
			break;
		}
		break;
	}
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

int read_fds_single(jc_dev_interface_t *dev, struct pollfd *pfd) {
	jc_device_t *self = (void*)dev;

	self->ui_revents = pfd[0].revents;
	self->jc_revents = pfd[1].revents;
	return 2;
}

int read_fds_dual(jc_dev_interface_t *dev, struct pollfd *pfd) {
	djc_device_t *self = (void*)dev;

	self->ui_revents = pfd[0].revents;
	self->left_revents = pfd[1].revents;
	self->right_revents = pfd[2].revents;
	return 3;
}

jc_device_t *jd_device_from_jc(joycon_t *jc) {
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
	return ret;
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
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_A, jc->buttons.left));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_B, jc->buttons.down));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_X, jc->buttons.right));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_Y, jc->buttons.up));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TL, jc->buttons.lsl));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TR, jc->buttons.lsr));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TL2, jc->buttons.l));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TR2, jc->buttons.zl));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_JOYSTICK, jc->buttons.ls));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_START, jc->buttons.minus));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_SELECT, jc->buttons.capture));
	} else {
		/* TODO: Maybe wrong */
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_A, jc->buttons.a));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_B, jc->buttons.b));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_X, jc->buttons.x));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_Y, jc->buttons.y));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TL, jc->buttons.rsl));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TR, jc->buttons.rsr));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TL2, jc->buttons.r));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TR2, jc->buttons.zr));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_JOYSTICK, jc->buttons.rs));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_START, jc->buttons.plus));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_SELECT, jc->buttons.home));
	}
	
	assert_null(libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0));

	return 1;
}

int jd_post_events_duo(djc_device_t *dev) {
	struct libevdev_uinput *uidev = dev->base.uidev;
	joycon_t *left = dev->left;
	joycon_t *right = dev->right;

	assert_null(libevdev_uinput_write_event(uidev, EV_ABS, 0, left->rstick[0]));
	assert_null(libevdev_uinput_write_event(uidev, EV_ABS, 1, left->rstick[1]));
	assert_null(libevdev_uinput_write_event(uidev, EV_ABS, 2, right->rstick[0]));
	assert_null(libevdev_uinput_write_event(uidev, EV_ABS, 3, right->rstick[1]));

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

	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_A, right->buttons.a));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_B, right->buttons.b));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_X, right->buttons.x));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_Y, right->buttons.y));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_0, left->buttons.up));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_1, left->buttons.down));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_2, left->buttons.left));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_3, left->buttons.right));

	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TL, left->buttons.l));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TR, right->buttons.r));

	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TL2, left->buttons.zl));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TR2, right->buttons.zr));

	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_THUMBL, left->buttons.ls));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_THUMBR, right->buttons.rs));

	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_START, right->buttons.plus));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_SELECT, left->buttons.minus));

	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TRIGGER_HAPPY + 0, left->buttons.capture));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TRIGGER_HAPPY + 1, right->buttons.home));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TRIGGER_HAPPY + 2, left->buttons.lsl));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TRIGGER_HAPPY + 3, left->buttons.lsr));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TRIGGER_HAPPY + 4, right->buttons.rsl));
	assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_TRIGGER_HAPPY + 5, right->buttons.rsr));

	assert_null(libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0));
	return 1;
}

djc_device_t *jd_device_from_jc2(joycon_t *left, joycon_t *right) {
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
	return ret;
}

int jd_wait_readable(int n, jc_dev_interface_t **devs) {
	jc_dev_interface_t *reordered[n];
	memset(reordered, 0, sizeof(reordered));
	int idx = 0;
	for (int i = 0; i < n; ++i) {
		if (devs[i])
			reordered[idx++] = devs[i];
	}

	struct pollfd fds[idx << 2];
	int nfds = 0;
	for(int i = 0; i < idx; ++i) {
		nfds += reordered[i]->write_fds(reordered[i], fds + nfds);
	}
	poll(fds, nfds, -1);
	nfds = 0;
	for(int i = 0; i < idx; ++i) {
		nfds += reordered[i]->read_fds(reordered[i], fds + nfds);
	}
	memcpy(devs, reordered, sizeof(reordered));
	return idx;
}
