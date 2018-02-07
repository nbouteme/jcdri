#include "single_dev.h"
#include <event_loop.h>
#include <cassert>

#ifdef assert_null
#undef assert_null
#endif

#define assert_null(x) {												\
		int __error_code = x;											\
		if (__error_code) {												\
			char *s = strerror(-__error_code);							\
			fprintf(stderr, "Failed at %s:%d with code %d %s\n", __FILE__, __LINE__, __error_code, s); \
			assert(0);													\
		}																\
	}																	\

namespace jcdri {

	bool single_dev::receive_event() {
		if (pfd.revents & (POLLHUP | POLLERR))
			el->remove_source(this);
		if (pfd.revents & POLLIN)
			handle_input_events();
		return false;
	}

	void single_dev::post_events() {
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

		if (jc->type == joycon::LEFT) {
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
	}

	void single_dev::play_effect(int id) {
		ff_effect *eff = rumble_patterns + id;
		switch(eff->type) {
		case FF_RUMBLE:;
			uint8_t rb[9] = {0};
			float str = eff->u.rumble.strong_magnitude;
			float wea = eff->u.rumble.weak_magnitude;

			rb[0] = next_tick();
			str /= 32768;
			wea /= 49152;
			make_rumble_data(320, 160, str + 0.4, wea + 0.4, rb + 1);
			memcpy(jc->rumble_data, rb + 1, 8);
			jc->set_vibration(1);
			break;
		}
	}

	void single_dev::stop_effect(int) {
		jc->set_vibration(0);
	}

	single_dev::single_dev(event_loop *el, std::shared_ptr<joycon> _jc)
		: jc_device(el), jc(_jc) {
		jc->jdev = this;
		char name[] = "EJoycon L";
		dev = 	libevdev_new();
		struct input_absinfo ain[2] = { {
				(int)jc->rstick[0],
				(int)jc->lcal.min_x, (int)jc->lcal.max_x,
				5, 10, 1
			}, {
				(int)jc->rstick[1],
				(int)jc->lcal.min_y, (int)jc->lcal.max_y,
				5, 10, 1
			} };

		struct input_absinfo abs =  {
			(int)jc->acc_comp.x,
			-32768, 32767,
			0, 0, 1
		};
		if (jc->type != joycon::LEFT) {
			name[8] = 'R';
			ain[0].minimum = jc->rcal.min_x;
			ain[0].maximum = jc->rcal.max_x;
			ain[1].minimum = jc->rcal.min_y;
			ain[1].maximum = jc->rcal.max_y;
		}

		libevdev_set_name(dev, name);
		libevdev_set_id_vendor(dev, 0x5652);
		libevdev_set_id_product(dev, 0x4432 + (jc->type != joycon::LEFT));

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
		assert_null(libevdev_uinput_create_from_device(dev, LIBEVDEV_UINPUT_OPEN_MANAGED, &uidev));
		uifd = libevdev_uinput_get_fd(uidev);
		pfd = {uifd, POLLIN | POLLOUT, 0};
	}
}