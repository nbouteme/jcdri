#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <sys/poll.h>

#include <vector>
#include <map>
#include <memory>

namespace jcdri {
	struct source;

	struct event_loop {
		std::vector<std::shared_ptr<source>> sources;
		std::map<source *, int> pfd_to_source;
		std::vector<pollfd> fds;
		bool should_stop = false;

		void append_source(std::shared_ptr<source> s);
		void remove_source(source *s);
		void run();
	};
}

#endif /* EVENTLOOP_H */