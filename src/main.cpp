#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <cassert>
#include <cstring>
#include <memory>

#include <sys/poll.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <unistd.h>

#include <waitable.h>
#include "udev.h"
#include <joycon.h>

#include <event_loop.h>
#include <ipc_source.h>

#include "devices/single_dev.h"
#include "devices/dual_dev.h"

#include <rumble.h>

namespace jcdri {

	struct joycon_source : public source {
		std::shared_ptr<joycon> jc;
		jc_device *dev = 0;

		joycon_source(event_loop *el, libudev::device &hidraw, libudev::device &hid) :
			source(el),
			jc(std::make_unique<joycon>(hidraw, hid)) {

			pfd = {jc->dev->device_handle, POLLIN, 0};
			uint8_t rb[8] = {0};
			make_rumble_data(320, 160, 0.5, 0.5, rb);
			memcpy(jc->rumble_data, rb, sizeof(rb));
			jc->set_player_leds(0xF0);
			jc->set_imu(1);
			jc->set_mode(joycon::jcinput::FULL);	
			jc->set_vibration(0);
			jc->read_calibration_data();
		}

		virtual bool receive_event() {
			// TODO: if POLLIN: poll, if POLLOUT, empty outgoing queue
			if (pfd.revents & (POLLHUP | POLLERR))
				el->remove_source(this);
			jc->poll();
			return false;
		}
	};

	struct udev_source : public source {
		libudev::monitor m;
		libudev::context c;
		std::vector<std::shared_ptr<joycon>> joycons;

		explicit udev_source(event_loop *el) : source(el) {
			m = c.from_netlink("udev");
			m.add_match_subsystem_devtype("hidraw", "");
			m.update();
			m.enable_receiving();
			pfd = {m.get_fd(), POLLIN, 0};
		}

		virtual bool receive_event() {
			auto hidraw = m.receive_device();
			auto hid = hidraw.get_parent_with_subsystem_devtype("hid", "");
			if (hidraw.get_action() != "add") {
				// TODO: si remove
				// trouver le joycon qui correspond
				// desactiver tout les devices l'utilisant
				// retirer de "joycons" 
				return false;
			}
			auto str = hid.get_property_value("HID_ID");
			if (str.find("0005:0000057E:0000200") != std::string::npos) {
				auto js = std::make_unique<joycon_source>(el, hidraw, hid);
				joycons.push_back(js->jc);
				el->append_source(std::move(js));
			}
			return false;
		}
	};
}

int main()
{
	using namespace std;
	using namespace jcdri;
	event_loop el;

	auto ipcs = make_shared<ipc_source>(&el);
	auto udevs = make_shared<udev_source>(&el);
	std::vector<jc_device*> devices;

	(*ipcs)[ipc_source::ECHO] = [&](auto client, auto data) {
		client->write(data.data(), data.size());
		client->close();
		return false;
	};

	(*ipcs)[ipc_source::GET_JOYCONS] = [&](auto client, auto data) {
		(void)data;
		ipc_source::response r;
		unsigned char buf[udevs->joycons.size() * 2];
		unsigned char *ptr = buf;
		for (unsigned i = 0; i < udevs->joycons.size(); ++i) {
			*ptr++ = i;
			*ptr++ = udevs->joycons[i]->type;
		}
		client->write(&r, sizeof(r));
		int s = sizeof(buf);
		client->write(&s, sizeof(s));
		client->write(buf, s);
		return false;
	};

	(*ipcs)[ipc_source::BUILD_DEVICE] = [&](auto client, auto data) {
		char *d = data.data();
		int type = d[0];
		ipc_source::response r;
		if (type == 0) {
			if (data.size() != 2
				|| (unsigned)d[1] >= udevs->joycons.size()) {
				r = ipc_source::ERROR;
				goto fail;
			}
			auto dev = std::make_shared<single_dev>(&el, udevs->joycons[d[1]]);
			el.append_source(dev);
		} else if (type == 1) {
			if (data.size() != 3
				|| (unsigned)d[1] >= udevs->joycons.size() 
				|| (unsigned)d[2] >= udevs->joycons.size()) {
				r = ipc_source::ERROR;
				goto fail;
			}
			auto dev = std::make_shared<dual_dev>(&el,
												  udevs->joycons[d[1]],
												  udevs->joycons[d[2]]);
			el.append_source(dev);
		} else {
			r = ipc_source::E_NO_SUCH;
		}
	fail:;
		client->write(&r, sizeof(r));
		return false;
	};

	(*ipcs)[ipc_source::GET_DEVICES] = [&](auto client, auto data) {
		(void)data;
		ipc_source::response r;
		unsigned char buf[devices.size() * 2];
		unsigned char *ptr = buf;
		for (unsigned i = 0; i < devices.size(); ++i) {
			*ptr++ = i;
			*ptr++ = typeid(devices[i]).hash_code();
		}
		client->write(&r, sizeof(r));
		int s = sizeof(buf);
		client->write(&s, sizeof(s));
		client->write(buf, s);
		return false;
	};

	(*ipcs)[ipc_source::DESTROY_DEVICE] = [&](auto client, auto data) {
		(void)client;
		(void)data;
		return false;
	};

	(*ipcs)[ipc_source::END_SESSION] = [&](auto client, auto data) {
		(void)data;
		client->close();
		return false;
	};
	
	(*ipcs)[ipc_source::EXIT] = [&](auto client, auto data) {
		(void)data;
		client->close();
		return true;
	};
	
	el.append_source(ipcs);
	el.append_source(udevs);
	el.run();
    return 0;
}
