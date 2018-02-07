#include <libudev.h>
#include <memory>
#include "udev.h"
#include <iostream>

namespace libudev {
	void context::set_userdata(void *d) {
		udev_set_userdata(_internal, d);
	}

	void *context::get_userdata() {
		return udev_get_userdata(_internal);
	}

	device context::from_syspath(const std::string &p) {
		return device(udev_device_new_from_syspath(_internal, p.c_str()));
	}
	
	device context::from_devnum(char type, dev_t devnum) {
		return device(udev_device_new_from_devnum(_internal, type, devnum));
	}

	device context::from_subsystem_sysname(const std::string &a, const std::string &b) {
		return device(udev_device_new_from_subsystem_sysname(_internal, a.c_str(), b.c_str()));
	}
	
	device context::from_device_id(const std::string &p) {
		return device(udev_device_new_from_syspath(_internal, p.c_str()));
	}

	device context::from_environment() {
		return device(udev_device_new_from_environment(_internal));
	}

	monitor context::from_netlink(const std::string &p) {
		return monitor(udev_monitor_new_from_netlink(_internal, p.c_str()));
	}

	enumerate context::new_enumerate() {
		return enumerate(udev_enumerate_new(_internal));
	}

	list_entry list_entry::next() {
		return list_entry(udev_list_entry_get_next(_internal));
	}

	list_entry list_entry::by_name(const std::string &p) {
		return list_entry(udev_list_entry_get_by_name(_internal, p.c_str()));
	}

	std::string list_entry::name() {
		auto p = udev_list_entry_get_name(_internal);
		return p ? p : "";
	}

	std::string list_entry::value(){
		auto p = udev_list_entry_get_value(_internal);
		return p ? p : "";
	}

	context device::get_udev() {
		return context(udev_device_get_udev(_internal));
	}

	device device::get_parent() {
		return device(udev_device_get_parent(_internal));
	}

	device device::get_parent_with_subsystem_devtype(const std::string &a, const std::string &b) {
		return device(udev_device_get_parent_with_subsystem_devtype(_internal, a.c_str(), b == "" ? 0 : b.c_str()));
	}

	std::string device::get_devpath() {
		auto p = udev_device_get_devpath(_internal);
		return p ? p : "";
	}

	std::string device::get_subsystem() {
		auto p = udev_device_get_subsystem(_internal);
		return p ? p : "";
	}

	std::string device::get_devtype() {
		auto p = udev_device_get_devtype(_internal);
		return p ? p : "";
	}

	std::string device::get_syspath() {
		auto p = udev_device_get_syspath(_internal);
		return p ? p : "";
	}

	std::string device::get_sysname() {
		auto p = udev_device_get_sysname(_internal);
		return p ? p : "";
	}

	std::string device::get_sysnum() {
		auto p = udev_device_get_sysnum(_internal);
		return p ? p : "";
	}

	std::string device::get_devnode() {
		auto p = udev_device_get_devnode(_internal);
		return p ? p : "";
	}

	bool device::is_initialized() {
		return udev_device_get_is_initialized(_internal);
	}

	list_entry device::get_devlinks() {
		return list_entry(udev_device_get_devlinks_list_entry(_internal));
	}

	list_entry device::get_properties() {
		return list_entry(udev_device_get_properties_list_entry(_internal));
	}

	list_entry device::get_tags() {
		return list_entry(udev_device_get_tags_list_entry(_internal));
	}

	list_entry device::get_sysattr() {
		return list_entry(udev_device_get_sysattr_list_entry(_internal));
	}

	std::string device::get_property_value(const std::string &k) {
		auto p = udev_device_get_property_value(_internal, k.c_str());
		return p ? p : "";
	}

	std::string device::get_driver() {
		auto p = udev_device_get_driver(_internal);
		return p ? p : "";
	}

	dev_t device::get_devnum() {
		return udev_device_get_devnum(_internal);
	}

	std::string device::get_action() {
		auto p = udev_device_get_action(_internal);
		return p ? p : "";
	}

	unsigned long long int device::get_seqnum() {
		return udev_device_get_seqnum(_internal);
	}

	unsigned long long int device::get_usec_since_initialized() {
		return udev_device_get_usec_since_initialized(_internal);
	}

	std::string device::get_sysattr_value(const std::string &k) {
		auto p = udev_device_get_sysattr_value(_internal, k.c_str());
		return p ? p : "";
	}

	bool device::has_tag(const std::string &t) {
		return udev_device_has_tag(_internal, t.c_str());
	}

	void print_list(device *d, const char *n, list_entry le) {
		printf("    %s:\n    {\n", n);
		while (le) {
			if (std::string(n) != "sysattr")
				printf("        %s: %s\n", le.name().c_str(), le.value().c_str());
			else {
				auto value = d->get_sysattr_value(le.name());
				printf("        %s: %s\n", le.name().c_str(), value.c_str());			
			}
			le = le.next();
		}
		puts("    }");
	}

	void device::print() {
		printf("this = %p {\n"
			   "    Devpath %s\n"
			   "    SubSystem %s\n"
			   "    DevType %s\n"
			   "    Syspath %s\n"
			   "    Sysname %s\n"
			   "    Sysnum %s\n"
			   "    Devnode %s\n"
			   "    Init? %d\n"
			   "    Driver %s\n"
			   "    Action %s\n",
			   this,
			   get_devpath().c_str(),
			   get_subsystem().c_str(),
			   get_devtype().c_str(),
			   get_syspath().c_str(),
			   get_sysname().c_str(),
			   get_sysnum().c_str(),
			   get_devnode().c_str(),
			   is_initialized(),
			   get_driver().c_str(),
			   get_action().c_str());
		print_list(this, "sysattr", get_sysattr());
		print_list(this, "properties", get_properties());
		print_list(this, "tags", get_tags());
		print_list(this, "devlinks", get_devlinks());
		puts("}");
	}

	context monitor::get_udev() {
		return context(udev_monitor_get_udev(_internal));
	}

	int monitor::enable_receiving() {
		return udev_monitor_enable_receiving(_internal);
	}

	int monitor::set_receive_buffer_size(int s) {
		return udev_monitor_set_receive_buffer_size(_internal, s);
	}

	int monitor::get_fd() {
		return udev_monitor_get_fd(_internal);
	}

	device monitor::receive_device() {
		auto p = udev_monitor_receive_device(_internal);
		return device(p);
	}

	int monitor::add_match_subsystem_devtype(const std::string &a, const std::string &b) {
		return udev_monitor_filter_add_match_subsystem_devtype(_internal, a.c_str(), b == "" ? 0 : b.c_str());
	}

	int monitor::add_match_tag(const std::string &a) {
		return udev_monitor_filter_add_match_tag(_internal, a.c_str());
	}

	int monitor::update() {
		return udev_monitor_filter_update(_internal);
	}

	int monitor::remove() {
		return udev_monitor_filter_remove(_internal);
	}

	context enumerate::get_udev() {
		return context(udev_enumerate_get_udev(_internal));
	}

	int enumerate::add_match_subsystem(const std::string &a) {
		return udev_enumerate_add_match_subsystem(_internal, a.c_str());
	}

	int enumerate::add_match_sysattr(const std::string &a, const std::string &b) {
		return udev_enumerate_add_match_sysattr(_internal, a.c_str(), b.c_str());
	}

	int enumerate::add_nomatch_subsystem(const std::string &a) {
		return udev_enumerate_add_nomatch_subsystem(_internal, a.c_str());
	}

	int enumerate::add_nomatch_sysattr(const std::string &a, const std::string &b) {
		return udev_enumerate_add_nomatch_sysattr(_internal, a.c_str(), b.c_str());
	}

	int enumerate::add_match_tag(const std::string &a) {
		return udev_enumerate_add_match_tag(_internal, a.c_str());
	}

	int enumerate::add_match_property(const std::string &a, const std::string &b) {
		return udev_enumerate_add_match_property(_internal, a.c_str(), b.c_str());
	}

	int enumerate::add_match_parent(device parent) {
		return udev_enumerate_add_match_parent(_internal, parent._internal);
	}

	int enumerate::add_match_is_initialized() {
		return udev_enumerate_add_match_is_initialized(_internal);
	}

	int enumerate::add_syspath(const std::string &s) {
		return udev_enumerate_add_syspath(_internal, s.c_str());
	}

	int enumerate::scan_devices() {
		return udev_enumerate_scan_devices(_internal);
	}

	int enumerate::scan_subsystems() {
		return udev_enumerate_scan_subsystems(_internal);
	}

	list_entry enumerate::get_list() {
		return list_entry(udev_enumerate_get_list_entry(_internal));
	}
}
