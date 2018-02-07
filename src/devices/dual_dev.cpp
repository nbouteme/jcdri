#include <devices/dual_dev.h>
#include <event_loop.h>
#include <limits.h>

namespace jcdri {

	bool dual_dev::receive_event() {
		if (pfd.revents & (POLLHUP | POLLERR))
			el->remove_source(this);
		if (pfd.revents & POLLIN)
			handle_input_events();
		return false;
	}

	void dual_dev::post_events() {
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
	}

	void dual_dev::play_effect(int id) {
		struct ff_effect *eff = rumble_patterns + id;
		switch(eff->type) {
		case FF_RUMBLE:;
			uint8_t rb[9] = {0};
			float str = eff->u.rumble.strong_magnitude;
			float wea = eff->u.rumble.weak_magnitude;

			rb[0] = next_tick();
			str /= 32768;
			wea /= 49152;
			make_rumble_data(320, 160, str + 0.4, wea + 0.4, rb + 1);
			memcpy(left->rumble_data, rb + 1, 8);
			memcpy(right->rumble_data, rb + 1, 8);
			left->set_vibration(1);
			right->set_vibration(1);
			break;
		}
	}

	void dual_dev::stop_effect(int) {
		left->set_vibration(0);
		right->set_vibration(0);
	}

	dual_dev::dual_dev(event_loop *el, std::shared_ptr<joycon> _left, std::shared_ptr<joycon> _right)
		: jc_device(el),
		  left(_left),
		  right(_right) {
		char name[] = "EJoycon Dual";
		left->jdev = right->jdev = this;
		dev = libevdev_new();
		struct input_absinfo base = {
			0, -100, 100, 0, 0, 1
		};
		(void)base;
		struct input_absinfo ain[4] = { {
				(int)left->rstick[0],
				(int)left->lcal.min_x, (int)left->lcal.max_x,
				(int)left->stick_prop.deadzone, 10, 1
			}, {
				(int)left->rstick[1],
				(int)left->lcal.min_y, (int)left->lcal.max_y,
				(int)left->stick_prop.deadzone, 10, 1
			}, {
				(int)right->rstick[0],
				(int)right->rcal.min_x, (int)right->rcal.max_x,
				(int)right->stick_prop.deadzone, 10, 1
			}, {
				(int)right->rstick[1],
				(int)right->rcal.min_y, (int)right->rcal.max_y,
				(int)right->stick_prop.deadzone, 10, 1
			}};

		struct input_absinfo abs =  {
			left->acc_comp.x,
			SHRT_MIN, SHRT_MAX,
			0, 0, 1
		};

		libevdev_set_name(dev, name);
		libevdev_set_id_vendor(dev, 0x5652);
		libevdev_set_id_product(dev, 0x4444);

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

		assert_null(libevdev_uinput_create_from_device(dev, LIBEVDEV_UINPUT_OPEN_MANAGED, &uidev));
		uifd = libevdev_uinput_get_fd(uidev);
		pfd = {uifd, POLLIN | POLLOUT, 0};
	}
}