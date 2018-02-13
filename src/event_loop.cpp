#include <event_loop.h>
#include <source.h>
#include <algorithm>
#include <cassert>

namespace jcdri {
	void event_loop::append_source(std::shared_ptr<source> s) {
		fds.push_back(s->pfd);
		sources.push_back(std::move(s));
	}

	void event_loop::remove_source(source *s) {
		auto fdsit = find_if(begin(fds), end(fds), [s](auto pfd){
			return pfd.fd == s->pfd.fd;
		});
		assert(fdsit != end(fds));
		fds.erase(fdsit);
		auto sit = find_if(begin(sources), end(sources), [s](auto o){
			return o.get() == s;
		});
		assert(sit != end(sources));
		sources.erase(sit);
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