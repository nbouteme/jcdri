#ifndef JOYCON_H
#define JOYCON_H

#include <stdint.h>
#include <hidapi.h>
#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>
#include <linux/uinput.h>
#include <sys/poll.h>

#include "bluetooth_hid.h"

#define PACKED __attribute__((packed))

// redefinition to hijack hidapi's handle
struct hid_device_ {
	int device_handle;
	int blocking;
	int uses_numbered_reports;
};

struct joycon_s;
typedef struct joycon_s joycon_t;

#define assert_null(x) {												\
		int __error_code = x;											\
		if (__error_code) {												\
			char *s = strerror(-__error_code);							\
			fprintf(stderr, "Failed at %s:%d with code %d %s\n", __FILE__, __LINE__, __error_code, s); \
			return 0;													\
		}																\
	}																	\

typedef enum {
	LEFT, RIGHT
} jctype_t;

typedef enum {
	UNKNOWN = 0,
	FULL = 0x30,
	STD = 0x3F
} jcinput_t;

typedef struct int32vec3_s {
	int32_t x, y, z;
} __attribute__((packed)) int32vec3_t;

typedef struct int12vec3_s {
	int x : 12;
	int y : 12;
	int z : 12;
} __attribute__((packed)) int12vec3_t;

typedef struct  {
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
} PACKED named_buttons_t;


typedef struct joycon_s {
	hid_device *dev;
	jctype_t type;
	jcinput_t mode;
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

	int16vec3_t acc_orig;
	int16vec3_t acc_sens;
	int16vec3_t gyro_orig;
	int16vec3_t gyro_sens;
	struct {
		unsigned deadzone : 12;
		unsigned range : 12;
	} stick_prop;
	int16vec3_t acc_comp;
	int16vec3_t gyro_comp;
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
} __attribute__((packed)) joycon_t;

joycon_t   *jc_get_joycon(jctype_t type);
void		jc_poll(joycon_t *jc);
int			jc_send_rcmd(joycon_t *jc, std_output_report_t *cmd);
int			jc_send_cmd(joycon_t *jc, std_output_report_t *cmd);
int			jc_send_rumble(joycon_t *jc, void *data);
void		jc_set_rumble(joycon_t *jc, int frequency, int intensity);
void		jc_set_input_mode(joycon_t *jc, jcinput_t mode);	
int			jc_get_player_leds(joycon_t *jc);
void		jc_set_player_leds(joycon_t *jc, uint8_t val);
void		jc_set_home_led(joycon_t *jc, uint8_t val);
void		jc_set_imu(joycon_t *jc, uint8_t val);
void		jc_set_vibration(joycon_t *jc, uint8_t val);
void		jc_read_calibration_data(joycon_t *jc);
void		jc_print_joycon(joycon_t *left);

typedef enum {
	SINGLE,
	DUO
} jc_playstyle_t;

typedef struct jc_dev_interface_s {
	struct libevdev *dev;
	struct libevdev_uinput *uidev;
	int uifd;
	struct ff_effect rumble_patterns[10];
	int used_patterns;
	void (*run_events)(struct jc_dev_interface_s *);
	int  (*write_fds)(struct jc_dev_interface_s *, struct pollfd *);
	int  (*read_fds)(struct jc_dev_interface_s *, struct pollfd *);
	void (*play_effect)(struct jc_dev_interface_s *, int);
	void (*stop_effect)(struct jc_dev_interface_s *, int);
} jc_dev_interface_t;

int jd_wait_readable(int n, jc_dev_interface_t **devs);
int next_tick();

#endif /* JOYCON_H */