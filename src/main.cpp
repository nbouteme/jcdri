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

#include <udev.h>
#include <joycon.h>

#include <event_loop.h>
#include <ipc_source.h>

#include <devices/single_dev.h>
#include <devices/dual_dev.h>
#include <devices/dual_keyboard.h>
#include <devices/mouse_dev.h>

#include <logger.h>

#include <rumble.h>

namespace jcdri {

	struct joycon_source : public source {
		std::shared_ptr<joycon> jc;
		jc_device *dev = 0;

		joycon_source(event_loop *el, const std::shared_ptr<joycon> &_jc) : source(el), jc(_jc) {
			printf("Connected joycon type %d\n", jc->type);
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

		joycon_source(event_loop *el, libudev::device &hidraw, const std::string &uniq) :
			joycon_source(el, std::make_shared<joycon>(hidraw, uniq)) {
		}

		joycon_source(event_loop *el, hid_device_info *info) :
			joycon_source(el, std::make_shared<joycon>(info)) {
		}

		virtual bool receive_event() {
			// TODO: if POLLIN: poll, if POLLOUT, empty outgoing queue
			if (pfd.revents & (POLLHUP | POLLERR)) {
				jc.reset();
				el->remove_source(this);
				return false;
			}
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
			auto str = hid.get_property_value("HID_ID");
			if (hidraw.get_action() == "remove") {
				auto it = std::find_if(begin(joycons), end(joycons), [&str](auto jc) {
						return str == jc->serial;
				});
				joycons.erase(it);
				puts("Erasing joycon from array");
				return false;
			}
			if (str.find("0005:0000057E:0000200") != std::string::npos) {
				auto js = std::make_unique<joycon_source>(el, hidraw, str);
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
	log(TRACE, "Starting program\n");
	auto ipcs = make_shared<ipc_source>(&el);
	auto udevs = make_shared<udev_source>(&el);
	vector<shared_ptr<jc_device>> devices;
	auto products = {0x2006, 0x2007};
	
	for (auto pid : products) {
		auto hdi = hid_enumerate(0x057E, pid);
		auto it_hdi = hdi;
		while (it_hdi) {
			auto js = make_shared<joycon_source>(&el, it_hdi);
			udevs->joycons.push_back(js->jc);
			el.append_source(js);
			it_hdi = it_hdi->next;
		}
		hid_free_enumeration(hdi);
	}

	(*ipcs)[ipc_source::ECHO] = [&](auto client, auto data) {
		client->write(data.data(), data.size());
		client->close();
		return false;
	};

	(*ipcs)[ipc_source::GET_JOYCONS] = [&](auto client, auto data) {
		(void)data;
		auto r = ipc_source::SUCCESS;
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

	map<ipc_source::dev_type, function<shared_ptr<jc_device>(const vector<char>&)>> device_creation_funs = {
		{ipc_source::SINGLE, [&](auto v) -> shared_ptr<jc_device> {
			return make_shared<single_dev>(&el, udevs->joycons[v[0]]);
		}},
		{ipc_source::DUAL, [&](auto v) -> shared_ptr<jc_device> {
			return make_shared<dual_dev>(&el, udevs->joycons[v[0]], udevs->joycons[v[1]]);
		}},
		{ipc_source::DUAL_KEYBOARD, [&](auto v) -> shared_ptr<jc_device> {
			return make_shared<dual_keyboard>(&el, udevs->joycons[v[0]], udevs->joycons[v[1]]);
		}},
		{ipc_source::MOUSE, [&](auto v) -> shared_ptr<jc_device> {
			return make_shared<mouse_dev>(&el, udevs->joycons[v[0]]);
		}},
	};

	(*ipcs)[ipc_source::BUILD_DEVICE] = [&](auto client, auto data) {
		char *d = data.data();
		auto type = ipc_source::dev_type(d[0]);
		data.erase(data.begin());
		auto r = ipc_source::ERROR;
		if (!any_of(data.begin(), data.end(), [&](auto d){return (unsigned)d >= udevs->joycons.size();})) {
			r = ipc_source::E_NO_SUCH;
			auto fun = device_creation_funs[type];
			if (fun) {
				shared_ptr<jc_device> dev = fun(data);
				dev->type = type;
				el.append_source(dev);
				devices.push_back(dev);
				r = ipc_source::SUCCESS;
			}
		}
		client->write(&r, sizeof(r));
		return false;
	};

	(*ipcs)[ipc_source::GET_DEVICES] = [&](auto client, auto data) {
		(void)data;
		ipc_source::response r = ipc_source::SUCCESS;
		unsigned char buf[devices.size() * 2];
		unsigned char *ptr = buf;
		for (unsigned i = 0; i < devices.size(); ++i) {
			*ptr++ = i;
			*ptr++ = devices[i]->type;
		}
		client->write(&r, sizeof(r));
		int s = sizeof(buf);
		client->write(&s, sizeof(s));
		client->write(buf, s);
		return false;
	};

	(*ipcs)[ipc_source::DESTROY_DEVICE] = [&](auto client, auto data) {
		ipc_source::response r = ipc_source::ERROR;
		if ((unsigned)data.front() < devices.size()) {
			r = ipc_source::SUCCESS;
			while (data.size()) {
				auto idx = data.front();
				data.erase(data.begin());
				auto ptr = devices[idx];
				el.remove_source(ptr.get());
				printf("Size before %ld\n", devices.size());
				devices.erase(devices.begin() + idx);
				printf("Size after %ld\n", devices.size());				
			}
		}
		client->write(&r, sizeof(r));
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
