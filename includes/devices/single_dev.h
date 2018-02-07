#ifndef SINGLE_DEV_H
#define SINGLE_DEV_H

#include <joycon.h>

namespace jcdri {
	struct single_dev : public jc_device {
		std::shared_ptr<joycon> jc;

		virtual bool receive_event() override;
		virtual void play_effect(int id) override;
		virtual void stop_effect(int) override;
		virtual void post_events() override;
		single_dev(event_loop *el, std::shared_ptr<joycon> _jc);
	};
}
#endif /* SINGLE_DEV_H */