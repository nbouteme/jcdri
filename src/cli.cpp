#include <sys/socket.h>
#include <sys/poll.h>
#include <linux/un.h>
#include <cstring>
#include <unistd.h>
#include <string>
#include <cassert>

int main(int argc, char **argv)
{
	using namespace std;

	if (argc < 2)
		return 1;

	int op = stoi(argv[1]);
	
	sockaddr_un uaddr;
	uaddr.sun_family = AF_UNIX;
	auto path = "/run/jcdri/ipc";
	memcpy(uaddr.sun_path, path, strlen(path) + 1);
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket");
		assert(fd >= 0);
	}
	if (connect(fd, (struct sockaddr *)&uaddr, sizeof(uaddr)) < 0) {
		perror("connect");
		assert(0);
	}

	if (op == 0) {
		// Operation: ECHO
		write(fd, &op, 4);
		// Size: 6
		int data = 6;
		write(fd, &data, 4);
		write(fd, "coucou", 6);
		char buf[7] = {0};
		read(fd, buf, 6);
		close(fd);
		assert(strcmp(buf, "coucou") == 0);
	} else if (op == 1) {
		write(fd, &op, 4);
		op = 0;
		write(fd, &op, 4);
		int status;
		read(fd, &status, 4);
		assert(status == 0);
		read(fd, &status, 4);
		char *jcbuf = new char[status];
		read(fd, jcbuf, status);
		for (int i = 0; i < status; i += 2) {
			printf("%d: %s\n", jcbuf[i], jcbuf[i + 1] ? "Left" : "Right");
		}
		delete[] jcbuf;
	} else if (op == 2) {
		char idx = 0;
		// BUILD DEVICE
		write(fd, &op, 4);
		op = 2;
		// 2 bytes of data
		write(fd, &op, 4);
		// type 0 (single)
		write(fd, &idx, 1);
		idx = stoi(argv[2]);
		// index
		write(fd, &idx, 1);
		int status;
		read(fd, &status, 4);
		assert(status == 0);
	}
	
    return 0;
}
