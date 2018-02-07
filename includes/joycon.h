#ifndef JOYCON_H
#define JOYCON_H

#include <stdint.h>
#include <hidapi.h>
#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>
#include <linux/uinput.h>
#include <sys/poll.h>
#include <memory>

#include <source.h>
#include <udev.h>
#include "rumble.h"

#include "bluetooth_hid.h"

// redefinition to hijack hidapi's handle
struct hid_device_ {
	int device_handle;
	int blocking;
	int uses_numbered_reports;
};

#define PACKED __attribute__((packed))

#include <stdio.h>
#include <errno.h>
#include <cstring>

#define assert_null(x) x

struct int32vec3 {
	int32_t x, y, z;
} PACKED;

struct int12vec3 {
	int x : 12;
	int y : 12;
	int z : 12;
} PACKED;

namespace jcdri {

	struct named_buttons {
		int y : 1;
		int x : 1;
		int b : 1;
		int a : 1;

		int rsr : 1;
		int rsl : 1;
		int r : 1;
		int zr : 1;

		int minus : 1;
		int plus : 1;
		int rs : 1;
		int ls : 1;

		int home : 1;
		int capture : 1;
		int unused : 1;
		int charging : 1;

		int down : 1;
		int up : 1;
		int right : 1;
		int left : 1;

		int lsr : 1;
		int lsl : 1;
		int l : 1;
		int zl : 1;
	} PACKED;

	struct jc_device;

	struct joycon {
		enum jctype {
			LEFT, RIGHT
		} type;

		enum jcinput {
			UNKNOWN = 0,
			FULL = 0x30,
			STD = 0x3F
		} mode;

		hid_device *dev;
		jc_device *jdev = 0;
		char *serial;

		union {
			struct {
				uint max_x : 12; uint max_y : 12;
				uint cen_x : 12; uint cen_y : 12;
				uint min_x : 12; uint min_y : 12;
			} PACKED lcal;
			struct {
				uint cen_x : 12; uint cen_y : 12;
				uint min_x : 12; uint min_y : 12;
				uint max_x : 12; uint max_y : 12;
			} PACKED rcal;
		};

		int16vec3 acc_orig;
		int16vec3 acc_sens;
		int16vec3 gyro_orig;
		int16vec3 gyro_sens;
		struct {
			unsigned deadzone : 12;
			unsigned range : 12;
		} stick_prop;
		int16vec3 acc_comp;
		int16vec3 gyro_comp;
		uint8_t rumble_data[8];
		uint8_t imu_enabled;
		uint rstick[2];

		union {
			struct {
				uint8_t connection : 4;
				uint8_t battery : 4;
			} PACKED;
			uint8_t rbs;
		};
	
		union {
			struct  {
				int y : 1;
				int x : 1;
				int b : 1;
				int a : 1;

				int rsr : 1;
				int rsl : 1;
				int r : 1;
				int zr : 1;

				int minus : 1;
				int plus : 1;
				int rs : 1;
				int ls : 1;

				int home : 1;
				int capture : 1;
				int unused : 1;
				int charging : 1;

				int down : 1;
				int up : 1;
				int right : 1;
				int left : 1;

				int lsr : 1;
				int lsl : 1;
				int l : 1;
				int zl : 1;
			} PACKED;
			int rbuttons;
		};

		void poll();
		std_input_report sync_send_cmd(std_output_report&);
		int send_rcmd(std_output_report&);
		int send_cmd(std_output_report&);
		int send_rumble(unsigned char *);
		int read_message(std_input_report &res);
		int set_rumble(int freq, int intensity);
		void set_mode(jcinput);
		int get_player_leds();
		void set_player_leds(uint8_t v);
		void set_home_led(uint8_t v);
		void set_imu(uint8_t v);
		void set_vibration(uint8_t v);
		void read_calibration_data();
		void print();

		joycon();
		joycon(libudev::device &raw, libudev::device &hid);
	} __attribute__((packed));

	joycon   *jc_get_joycon(joycon::jctype type);

	typedef enum {
		SINGLE,
		DUO
	} jc_playstyle_t;

	struct jc_device : public source {
		virtual bool receive_event() override = 0;
		virtual void play_effect(int) = 0;
		virtual void stop_effect(int) = 0;
		virtual void post_events() = 0;

		void upload_effect(struct ff_effect effect);
		void erase_effect(int id_effect);
		void handle_input_events();

	jc_device(event_loop *e) : source(e) {}
		struct libevdev *dev;
		struct libevdev_uinput *uidev;
		int uifd;
		struct ff_effect rumble_patterns[10];
		int used_patterns;
	};

	int next_tick();
}

#endif /* JOYCON_H */
