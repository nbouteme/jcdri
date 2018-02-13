#include <ipc_source.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <linux/un.h>
#include <cstring>
#include <unistd.h>
#include <string>
#include <cassert>
#include <map>
#include <functional>

void print_help() {
	puts(
		"Usage: jccli [operation] [args...]\n"
		"\n"
		"Operations:\n"
		"lsjc			Prints a list of joycons in the form of ID: TYPE\n"
		"identify [ID]		Vibrates a joycon	\n"
		"build [type] [ID...]	Builds a device of a given type using the given joycons\n"
		"lsdev			Prints a list of created devices in the form of ID: TYPE\n"
		"destroy [ID]		Manually removes a device with the given ID.\n"
		"exit			Terminates the driver\n"
		"\n"
		"Device types:\n"
		"single			Maps a joycon to a 11 buttons 8 axis device\n"
		"dual			Maps a joycon pair to a 22 buttons 16 axis device\n"
		"dual_keyboard		Maps a joycon pair to 26 distinct keys\n"
		"mouse			Maps a joycon to a pointing device\n"
		"custom [file]		Maps a joycon according to a map file\n"
		"dual_custom [file]	Maps a joycon pair according to a map file"
	);
}

int main(int argc, char **argv)
{
	using namespace std;

	if (argc < 2) {
		print_help();
		return 1;
	}

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

	map<string, jcdri::ipc_source::command> str_to_op = {
		{"echo"		, jcdri::ipc_source::ECHO},
		{"lsjc"		, jcdri::ipc_source::GET_JOYCONS},
		{"lsdev"	, jcdri::ipc_source::GET_DEVICES},
		{"build"	, jcdri::ipc_source::BUILD_DEVICE},
		{"destroy"	, jcdri::ipc_source::DESTROY_DEVICE},
		{"exit"		, jcdri::ipc_source::EXIT},
	};

	map<string, jcdri::ipc_source::dev_type> str_to_type = {
		{"single"			, jcdri::ipc_source::SINGLE},
		{"dual"				, jcdri::ipc_source::DUAL},
		{"dual_keyboard"	, jcdri::ipc_source::DUAL_KEYBOARD},
		{"mouse"			, jcdri::ipc_source::MOUSE},
		{"custom"			, jcdri::ipc_source::CUSTOM},
		{"dual_custom"		, jcdri::ipc_source::DUAL_CUSTOM},
	};

	auto op = str_to_op[argv[1]];
	write(fd, &op, 4);
	map<string, function<int(void)>> funs = {
		{"echo", [&]()
		{
			// Size: 6
			int data = 6;
			write(fd, &data, 4);
			write(fd, "coucou", 6);
			char buf[7] = {0};
			read(fd, buf, 6);
			close(fd);
			return strcmp(buf, "coucou");
		}},
		{"lsjc", [&]()
		{
			int status = 0;
			write(fd, &status, 4);
			read(fd, &status, 4);
			assert(status == jcdri::ipc_source::SUCCESS);
			read(fd, &status, 4);
			char *jcbuf = new char[status];
			read(fd, jcbuf, status);
			for (int i = 0; i < status; i += 2) {
				printf("%d: %s\n", jcbuf[i], jcbuf[i + 1] ? "Right" : "Left");
			}
			delete[] jcbuf;
			return 0;
		}},
		{"lsdev", [&]()
		{
			int status = 0;
			write(fd, &status, 4);
			read(fd, &status, 4);
			printf("status %d\n", status);
			assert(status == jcdri::ipc_source::SUCCESS);
			read(fd, &status, 4);
			char *devbuf = new char[status];
			read(fd, devbuf, status);
			for (int i = 0; i < status; i += 2) {
				auto pair = find_if(str_to_type.begin(), str_to_type.end(), [&](auto e){
						return e.second == devbuf[i + 1];
				});
				if (pair == str_to_type.end()) {
					printf("Couldn't find type %d\n", devbuf[i + 1]);
					continue;
				}
				printf("%d: %s\n", devbuf[i], pair->first.c_str());
			}
			delete[] devbuf;
			return 0;
		}},
		{"build", [&]()
		{
			vector<char> data;
			data.push_back(str_to_type[argv[2]]);
			char **b = argv + 3;
			while (*b) {
				data.push_back(stoi(*b++));
			}
			unsigned idx = data.size();
			write(fd, &idx, 4);
			write(fd, data.data(), data.size());
			int status;
			read(fd, &status, 4);
			assert(status == jcdri::ipc_source::SUCCESS);
			return 0;
		}},
		{"destroy", [&]()
		{
			vector<char> data;
			char **b = argv + 2;
			while (*b) {
				data.push_back(stoi(*b++));
			}
			unsigned idx = data.size();
			write(fd, &idx, 4);
			write(fd, data.data(), data.size());
			int status;
			read(fd, &status, 4);
			assert(status == jcdri::ipc_source::SUCCESS);
			return 0;
		}},
	};

	if (!funs[argv[1]]) {
		puts("Operation not implemented");
		print_help();
		return 1;
	}

	return funs[argv[1]]();
}
