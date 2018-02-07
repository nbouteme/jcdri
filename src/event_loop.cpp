#include <event_loop.h>
#include <source.h>

namespace jcdri {
	void event_loop::append_source(std::shared_ptr<source> s) {
		fds.push_back(s->pfd);
		pfd_to_source[s.get()] = fds.size() - 1;
		sources.push_back(std::move(s));
	}

	void event_loop::remove_source(source *s) {
		auto ptr = pfd_to_source[s];
		pfd_to_source.erase(s);
		fds.erase(begin(fds) + ptr);
		sources.erase(begin(sources) + ptr);
	}

	void event_loop::run() {
		while(!should_stop) {
			int n = poll(fds.data(), fds.size(), -1);
			if (n < 0)
				break;
			for (int i = 0; n; ++i)
				if (fds[i].revents) {
					sources[i]->pfd = fds[i];
					should_stop |= sources[i]->receive_event();
					--n;
				}
		}
	}
}