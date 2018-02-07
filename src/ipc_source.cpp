#include <ipc_source.h>
#include <cassert>

namespace jcdri {
	ipc_source::client::client(event_loop *el, ipc_source *_ipcs, int _fd) :
		source(el),
		ipcs(_ipcs) {
		pfd = {_fd, POLLIN, 0};
	}

	void ipc_source::client::write(void *data, size_t size) {
		::write(pfd.fd, data, size);
	}

	std::vector<unsigned char> ipc_source::client::read(size_t size, bool all) {
		std::vector<unsigned char> ret;
		auto left = size;
		ret.reserve(size);
		left -= ::read(pfd.fd, ret.data(), size);
		if (!all)
			return ret;
		while (left) {
			left -= ::read(pfd.fd, ret.data() + (size - left), left);
		}
		return ret;
	}

	void ipc_source::client::close() {
		el->remove_source(this);
		::close(pfd.fd);
	}
		
	bool ipc_source::client::receive_event() {
		int client = pfd.fd;
		int op;
		unsigned sz;
		std::vector<char> data;
		bool ret = false;

		if (pfd.revents & POLLERR || pfd.revents & POLLHUP) {
			el->remove_source(this);
			return false;
		}
		// TODO: Preserve state and return if not enough data available
		int n = 0;
		n = ::read(client, &op, sizeof(op));
		if (n < 0) {
			perror("read()");
			assert(0);
		}
		printf("%s:%d -> read %d bytes\n", __FILE__, __LINE__, n);
		n = ::read(client, &sz, sizeof(sz));
		if (n < 0) {
			perror("read()");
			assert(0);
		}
		printf("%s:%d -> read %d bytes\n", __FILE__, __LINE__, n);
		data.resize(sz);
		n = ::read(client, data.data(), data.size());
		if (n < 0) {
			perror("read()");
			assert(0);
		}
		printf("%s:%d -> read %d bytes\n", __FILE__, __LINE__, n);
		if (ipcs->handlers[op])
			ret = ipcs->handlers[op](this, data);
		return ret;
	}

	ipc_source::ipc_source(event_loop *_el) : source(_el) {
		sockaddr_un uaddr;
		uaddr.sun_family = AF_UNIX;
		auto path = "/run/jcdri/ipc";
		memcpy(uaddr.sun_path, path, strlen(path) + 1);
		int fd = socket(AF_UNIX, SOCK_STREAM, 0);
		unlink(path);
		bind(fd, (sockaddr*)&uaddr, sizeof(uaddr));
		listen(fd, 2);
		pfd = {fd, POLLIN, 0};
	}

	bool ipc_source::receive_event() {
		if (pfd.revents & POLLERR || pfd.revents & POLLHUP) {
			el->remove_source(this);
			return true;
		}

		sockaddr_un cliaddr;
		unsigned csize = sizeof(cliaddr);
		int clientfd = accept(pfd.fd, (sockaddr*)&cliaddr, &csize);
		el->append_source(std::make_unique<client>(el, this, clientfd));
		return false;
	}

	ipc_source::fun_type &ipc_source::operator[](int idx) {
		return handlers[idx];
	}
}