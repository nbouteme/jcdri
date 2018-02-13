#ifndef UDEV_H
#define UDEV_H

#include <libudev.h>
#include <memory>

namespace libudev {
	struct device;
	struct monitor;
	struct enumerate;

	struct context {
		struct udev *_internal;

		explicit context(struct udev *a) : _internal(a) {}

		context() : _internal(udev_new()) {}

		context(const context &rhs) : _internal(rhs._internal) {
			udev_ref(_internal);
		}

		context &operator=(const context &rhs) {
			_internal = rhs._internal;
			udev_ref(_internal);
			return *this;
		}

		void set_userdata(void *d);
		void *get_userdata();
		device from_syspath(const std::string &);
		device from_devnum(char type, dev_t devnum);
		device from_subsystem_sysname(const std::string &, const std::string &);
		device from_device_id(const std::string &);
		device from_environment();

		monitor from_netlink(const std::string &);
		enumerate new_enumerate();

		~context() { udev_unref(_internal); }
	};

	struct list_entry {
		struct udev_list_entry *_internal;

		explicit list_entry(struct udev_list_entry *a) : _internal(a) {}

		list_entry next();
		list_entry by_name(const std::string &);
		operator bool() {
			return !!_internal;
		}

		std::string name();
		std::string value();
	};

	struct device {
		struct udev_device *_internal;

		explicit device(struct udev_device *a) : _internal(a) {
		}

		device(const device &rhs) : _internal(rhs._internal) {
			udev_device_ref(_internal);
		}

		device &operator=(const device &rhs) {
			_internal = rhs._internal;
			udev_device_ref(_internal);
			return *this;
		}

		context get_udev();

		device get_parent();
		device get_parent_with_subsystem_devtype(const std::string &, const std::string &);

		std::string get_devpath();
		std::string get_subsystem();
		std::string get_devtype();
		std::string get_syspath();
		std::string get_sysname();
		std::string get_sysnum();
		std::string get_devnode();

		bool is_initialized();

		list_entry get_devlinks();
		list_entry get_properties();
		list_entry get_tags();
		list_entry get_sysattr();

		std::string get_property_value(const std::string &k);
		std::string get_driver();
		dev_t get_devnum();
		std::string get_action();
		unsigned long long int get_seqnum();
		unsigned long long int get_usec_since_initialized();
		std::string get_sysattr_value(const std::string &k);
		bool has_tag(const std::string &t);
		void print();

		~device() {
			udev_device_unref(_internal);
		}
	};

	struct monitor {
		context get_udev();
		struct udev_monitor *_internal;

		explicit monitor(struct udev_monitor *a) : _internal(a) {}

		monitor() : _internal(0) {
		}

		monitor(const monitor &rhs) : _internal(rhs._internal) {
			udev_monitor_ref(_internal);
		}

		monitor &operator=(const monitor &rhs) {
			_internal = rhs._internal;
			udev_monitor_ref(_internal);
			return *this;
		}

		int enable_receiving();
		int set_receive_buffer_size(int s);
		int get_fd();
		device receive_device();
		int add_match_subsystem_devtype(const std::string &, const std::string &);
		int add_match_tag(const std::string &);
		int update();
		int remove();

		~monitor() { udev_monitor_unref(_internal); }
	};

	struct enumerate {
		struct udev_enumerate *_internal;

		explicit enumerate(struct udev_enumerate *a) : _internal(a) {}

		enumerate(const enumerate &rhs) : _internal(rhs._internal) {
			udev_enumerate_ref(_internal);
		}

		enumerate &operator=(const enumerate &rhs) {
			_internal = rhs._internal;
			udev_enumerate_ref(_internal);
			return *this;
		}

		context get_udev();
		int add_match_subsystem(const std::string &);
		int add_match_sysattr(const std::string &, const std::string &);
		int add_nomatch_subsystem(const std::string &);
		int add_nomatch_sysattr(const std::string &, const std::string &);
		int add_match_tag(const std::string &);
		int add_match_property(const std::string &, const std::string &);
		int add_match_parent(device parent);
		int add_match_is_initialized();
		int add_syspath(const std::string &syspath);

		int scan_devices();
		int scan_subsystems();

		list_entry get_list();

		~enumerate() { udev_enumerate_unref(_internal); }
	};
}

#endif /* UDEV_H */
