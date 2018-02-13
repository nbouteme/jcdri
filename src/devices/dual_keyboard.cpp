#include <devices/dual_keyboard.h>
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
	bool dual_keyboard::receive_event() {
		if (pfd.revents & (POLLHUP | POLLERR))
			el->remove_source(this);
		if (pfd.revents & POLLIN)
			handle_input_events();
		return false;
	}

	void dual_keyboard::play_effect(int) {
	}

	void dual_keyboard::stop_effect(int) {
	}

	void dual_keyboard::post_events() {
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_A, left->left));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_B, left->up));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_C, left->down));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_D, left->right));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_E, left->lsl));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_F, left->lsr));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_G, left->ls));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_H, left->l));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_I, left->zl));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_J, left->capture));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_K, left->rstick[0] > 3000));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_L, left->rstick[0] < 900));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_M, left->rstick[1] > 3200));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_N, left->rstick[1] < 1200));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_O, left->minus));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_P, right->a));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_Q, right->b));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_R, right->x));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_S, right->y));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_T, right->rsl));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_U, right->rsr));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_V, right->rs));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_W, right->r));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_X, right->zr));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_Y, right->home));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_Z, right->rstick[0] > 3100));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_0, right->rstick[0] < 900));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_1, right->rstick[1] > 3200));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_2, right->rstick[1] < 1200));
		assert_null(libevdev_uinput_write_event(uidev, EV_KEY, KEY_3, right->plus));
		assert_null(libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0));
	}

	dual_keyboard::dual_keyboard(event_loop *el, const std::shared_ptr<joycon> &_left , const std::shared_ptr<joycon> &_right) :
		jc_device(el), left(_left), right(_right) {
		char name[] = "Joycon KBD";
		dev = libevdev_new();
		left->jdev = right->jdev = this;
		libevdev_set_name(dev, name);
		libevdev_set_id_vendor(dev, 0x5653);
		libevdev_set_id_product(dev, 0x4432);

		assert_null(libevdev_enable_property(dev, INPUT_PROP_BUTTONPAD));

		assert_null(libevdev_enable_event_type(dev, EV_KEY));
		// 11 boutons par manette
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_A, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_B, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_C, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_D, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_E, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_F, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_G, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_H, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_I, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_J, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_K, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_L, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_M, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_N, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_O, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_P, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_Q, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_R, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_S, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_T, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_U, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_V, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_W, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_X, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_Y, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_Z, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_0, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_1, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_2, NULL));
		assert_null(libevdev_enable_event_code(dev, EV_KEY, KEY_3, NULL));

		assert_null(libevdev_uinput_create_from_device(dev, LIBEVDEV_UINPUT_OPEN_MANAGED, &uidev));
	}
}
