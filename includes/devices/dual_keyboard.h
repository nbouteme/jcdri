#ifndef DUAL_KEYBOARD_H
#define DUAL_KEYBOARD_H

#include <joycon.h>

namespace jcdri {
	struct dual_keyboard : public jc_device {
		std::shared_ptr<joycon> left;
		std::shared_ptr<joycon> right;

		virtual bool receive_event() override;
		virtual void play_effect(int id) override;
		virtual void stop_effect(int) override;
		virtual void post_events() override;
		dual_keyboard(event_loop *el, const std::shared_ptr<joycon> &_left,
						  const std::shared_ptr<joycon> &_right);
		~dual_keyboard() {
			left->jdev = 0;
			right->jdev = 0;
		}
	};
}

#endif /* DUAL_KEYBOARD_H */
