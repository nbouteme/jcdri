#ifndef MOUSE_DEV_H
#define MOUSE_DEV_H

#include <joycon.h>

namespace jcdri {
	struct mouse_dev : public jc_device {
		std::shared_ptr<joycon> jc;

		virtual bool receive_event() override;
		virtual void play_effect(int id) override;
		virtual void stop_effect(int) override;
		virtual void post_events() override;
		mouse_dev(event_loop *el, const std::shared_ptr<joycon> &_jc);
		~mouse_dev() {
			jc->jdev = 0;
		}
	};
}


#endif /* MOUSE_DEV_H */