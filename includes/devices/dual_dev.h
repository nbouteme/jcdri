#ifndef DUAL_DEV_H
#define DUAL_DEV_H

#include <joycon.h>

namespace jcdri {
	struct dual_dev : public jc_device {
		std::shared_ptr<joycon> left;
		std::shared_ptr<joycon> right;

		virtual bool receive_event() override;
		virtual void play_effect(int id) override;
		virtual void stop_effect(int) override;
		virtual void post_events() override;
		dual_dev(event_loop *el, std::shared_ptr<joycon> _left, std::shared_ptr<joycon> _right);
	};
}
#endif /* DUAL_DEV_H */