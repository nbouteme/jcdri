#include <devices/mouse_dev.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
	bool mouse_dev::receive_event() {
		if (pfd.revents & (POLLHUP | POLLERR))
			el->remove_source(this);
		if (pfd.revents & POLLIN)
			handle_input_events();
		return false;
	}

	void mouse_dev::play_effect(int) {
	}

	void mouse_dev::stop_effect(int) {
	}

	void mouse_dev::post_events() {
		bool must_move = false;
		if (jc->type == joycon::LEFT) {
			must_move = jc->zl;
			assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_LEFT, 0));
			assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_MIDDLE, 0));
			assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_RIGHT, 0));
		} else {
			must_move = jc->zr;
			assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_LEFT, jc->a));
			assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_MIDDLE, jc->rs));
			assert_null(libevdev_uinput_write_event(uidev, EV_KEY, BTN_RIGHT, jc->b));
		}
		if (must_move) {
#define ABS(x) (x < 0 ? -x : x)
			int adjusted_z_comp = jc->gyro_comp.z + 5;
			if (ABS(adjusted_z_comp) > 5)
				assert_null(libevdev_uinput_write_event(uidev, EV_REL, REL_X, adjusted_z_comp / 10));
			int adjusted_y_comp = jc->gyro_comp.y + 35;
			if (ABS(adjusted_y_comp) > 5)
				assert_null(libevdev_uinput_write_event(uidev, EV_REL, REL_Y, (-adjusted_y_comp) / 10));
#undef ABS
		}
		assert_null(libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0));
	}

	mouse_dev::mouse_dev(event_loop *el, const std::shared_ptr<joycon> &_jc) :
		jc_device(el), jc(_jc) {
		char name[] = "Joycon Pointer";
		dev = libevdev_new();
		jc->jdev = this;
		jc->set_player_leds(0x09);
		puts("Build mouse");
		libevdev_set_name(dev, name);
		libevdev_set_id_vendor(dev, 0x5653);
		libevdev_set_id_product(dev, 0x4332);

		//assert_null(libevdev_enable_property(dev, INPUT_PROP_POINTING_STICK));
		assert_null(libevdev_enable_property(dev, INPUT_PROP_POINTER));
		//assert_null(libevdev_enable_property(dev, INPUT_PROP_ACCELEROMETER));

		assert_null(libevdev_enable_event_type(dev, EV_KEY));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_LEFT, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_MIDDLE, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, BTN_RIGHT, NULL));

		assert_null(libevdev_enable_event_type(dev, EV_REL));

		assert_null(libevdev_enable_event_code(dev, EV_REL, REL_X, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_REL, REL_Y, NULL));
		assert_null(libevdev_uinput_create_from_device(dev, LIBEVDEV_UINPUT_OPEN_MANAGED, &uidev));
	}
}