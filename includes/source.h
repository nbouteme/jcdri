#ifndef SOURCE_H
#define SOURCE_H

#include <sys/poll.h>

namespace jcdri {
	struct source;
	struct event_loop;

	struct source {
		event_loop *el;
		pollfd pfd;
		source(event_loop *_el) : el(_el) {}
		virtual bool receive_event() = 0;
		virtual ~source(){}
	};
}
#endif /* SOURCE_H */