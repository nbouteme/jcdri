#include <sys/poll.h>
#include <cstdio>
#include <cstring>
#include "udev.h"

using namespace std;
using namespace libudev;

void print_list(device &d, const char *n, list_entry le) {
	printf("    %s:\n    {\n", n);
	while (le) {
		if (string(n) != "sysattr")
			printf("        %s: %s\n", le.name().c_str(), le.value().c_str());
		else {
			auto value = d.get_sysattr_value(le.name());
			printf("        %s: %s\n", le.name().c_str(), value.c_str());			
		}
		le = le.next();
	}
	puts("    }");
}

void print_device(device d) {
	printf("{\n");
	printf("    Device Path %s\n"
		   "    Subsys %s\n"
		   "    DevType %s\n"
		   "    Syspath %s\n"
		   "    Sysname %s\n"
		   "    Sysnum %s\n"
		   "    Devnode %s\n"
		   "    Driver %s\n"
		   "    Action %s\n\n\n",
		   d.get_devpath().c_str(),
		   d.get_subsystem().c_str(),
		   d.get_devtype().c_str(),
		   d.get_syspath().c_str(),
		   d.get_sysname().c_str(),
		   d.get_sysnum().c_str(),
		   d.get_devnode().c_str(),
		   d.get_driver().c_str(),
		   d.get_action().c_str()
		);
	print_list(d, "properties", d.get_properties());
	print_list(d, "tags", d.get_tags());
	print_list(d, "sysattr", d.get_sysattr());
	print_list(d, "devlinks", d.get_devlinks());
	printf("}\n");
}
/*
int main()
{
	context c;

	auto m = c.from_netlink("udev");
	//m.add_match_subsystem_devtype("hidraw", "");
	//m.update();
	m.enable_receiving();
	int fd = m.get_fd();
	struct pollfd pfds = {fd, POLLIN, 0};
	int i = 0;
	while (i < 100) {
		poll(&pfds, 1, -1);
		auto d = m.receive_device();

		print_device(d);
		++i;
	}
    return 0;
}
*/


#include <hidapi/hidapi.h>

int main() {
	auto devs = hid_enumerate(0x57e, 0x2006);

	auto p = hid_open_path("/devices/pci0000:00/0000:00:14.0/usb1/1-5/1-5:1.0/bluetooth/hci0/hci0:17/0005:057E:2006.000B");
	if (!p)
		p = hid_open_path("/sys/devices/pci0000:00/0000:00:14.0/usb1/1-5/1-5:1.0/bluetooth/hci0/hci0:17/0005:057E:2006.000B");
	printf("keke\n");
}