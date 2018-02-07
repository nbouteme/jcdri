#ifndef IPCSOURCE_H
#define IPCSOURCE_H

#include <source.h>
#include <event_loop.h>

#include <unistd.h>
#include <sys/socket.h>
#include <linux/un.h>

#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <functional>

namespace jcdri {

	struct ipc_source : public source {
		struct client : public source {
			ipc_source *ipcs;
			client(event_loop *el, ipc_source *_ipcs, int _fd);
			void write(void *data, size_t size);
			std::vector<unsigned char> read(size_t size, bool all = true);
			void close();		
			virtual bool receive_event();
		};

		using fun_type = std::function<bool(client *, const std::vector<char>&)>;
		std::map<int, fun_type> handlers;
		std::map<int, std::unique_ptr<client>> clients;

		explicit ipc_source(event_loop *_el);
		virtual bool receive_event();
		fun_type &operator[](int idx);

		enum command {
			ECHO,
			GET_JOYCONS,
			BUILD_DEVICE,
			GET_DEVICES,
			DESTROY_DEVICE,
			END_SESSION,
			EXIT
		};

		enum response {
			E_NO_SUCH = -2,
			ERROR = -1,
			EVENT,
			SUCCESS,
		};
	};
}
#endif /* IPCSOURCE_H */