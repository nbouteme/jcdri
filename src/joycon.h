#ifndef JOYCON_H
#define JOYCON_H

#include <stdint.h>
#include <hidapi.h>

struct hid_device_ {
	int device_handle;
	int blocking;
	int uses_numbered_reports;
};

#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>
#include <linux/uinput.h>
#include <sys/poll.h>

struct joycon_s;
typedef struct joycon_s joycon_t;

#define PACKED __attribute__((packed))

typedef enum {
	LEFT, RIGHT
} jctype_t;

typedef enum {
	UNKNOWN = 0,
	FULL = 0x30,
	STD = 0x3F
} jcinput_t;

struct subcommand_reply_s {
	uint8_t ack;
	uint8_t reply_to;
	uint8_t data[35];	
} PACKED;

typedef struct frame_data_s {
	uint8_t data[38];	
} PACKED frame_data_t;

typedef struct int16vec3_s {
	int16_t x, y, z;
} PACKED int16vec3_t;

typedef struct imu_frame_s {
	int16vec3_t acc;
	int16vec3_t gyro;
} PACKED imu_frame_t;

typedef struct full_input_s {
	imu_frame_t frames[3];
} PACKED full_input_t;

typedef struct subcommand_reply_s subcommand_reply_t;

typedef struct std_full_report_s {
	uint8_t timer;
	uint ci : 4;
	uint bs : 4;
	uint buttons : 24;
	uint left_stick_x : 12;
	uint left_stick_y : 12;
	uint right_stick_x : 12;
	uint right_stick_y : 12;
	uint vibrator_report : 8;
	union {
		subcommand_reply_t reply;
		full_input_t imu_samples;
	};
} PACKED std_full_report_t;

typedef struct std_small_report_s {
	uint buttons : 16;
	uint stick : 8;
	uint8_t filler[8];
} PACKED std_small_report_t;

typedef struct std_input_report_s {
	uint8_t id;
	union {
		std_small_report_t std;
		std_full_report_t full;
	};
} PACKED std_input_report_t;

typedef struct spi_read_args_s {
	uint32_t addr;
	uint8_t size;
} PACKED spi_read_args_t;

typedef struct {
	uint unused : 4;
	uint timer : 4;
	uint8_t rumble_data[8];
	uint8_t sub_cmd;
	union {
		uint8_t rargs[38];
		spi_read_args_t spi_read_args;
		/* TODO: Add more argument types */
	};
} PACKED output_cmd_t;

typedef struct {
	uint unused : 4;
	uint timer : 4;
	uint8_t rumble_data[8];
} PACKED rumble_cmd_t;

typedef struct std_output_report_s {
	uint8_t id;
	union {
		output_cmd_t cmd;
		rumble_cmd_t rumble;
	};
} PACKED std_output_report_t;

typedef struct named_buttons_s {
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
} named_buttons_t;

struct battery_status_s {
	uint8_t connection : 4;
	uint8_t battery : 4;
} __attribute__((packed));

typedef struct left_stick_calibration_s {
	uint max_x : 12; uint max_y : 12;
	uint cen_x : 12; uint cen_y : 12;
	uint min_x : 12; uint min_y : 12;
} __attribute__((packed)) left_stick_calibration_t;

typedef struct right_stick_calibration_s {
	uint cen_x : 12; uint cen_y : 12;
	uint min_x : 12; uint min_y : 12;
	uint max_x : 12; uint max_y : 12;
} __attribute__((packed)) right_stick_calibration_t;

typedef struct int32vec3_s {
	int32_t x, y, z;
} __attribute__((packed)) int32vec3_t;

typedef struct int12vec3_s {
	int x : 12;
	int y : 12;
	int z : 12;
} __attribute__((packed)) int12vec3_t;

typedef struct joycon_s {
	hid_device *dev;
	jctype_t type;
	jcinput_t mode;

	union {
		left_stick_calibration_t lcal;
		right_stick_calibration_t rcal;
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
	int rstick[2];

	union {
		struct battery_status_s bs;
		uint8_t rbs;
	};
	
	union {
		named_buttons_t buttons;
		int rbuttons;
	};
} __attribute__((packed)) joycon_t;

joycon_t   *jc_get_joycon(jctype_t type);
void		jc_poll(joycon_t *jc);
int			jc_send_rcmd(joycon_t *jc, std_output_report_t *cmd);
int jc_send_cmd(joycon_t *jc, std_output_report_t *cmd);
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
	int (*write_fds)(struct jc_dev_interface_s *, struct pollfd *);
	int (*read_fds)(struct jc_dev_interface_s *, struct pollfd *);
	void (*play_effect)(struct jc_dev_interface_s *, int);
	void (*stop_effect)(struct jc_dev_interface_s *, int);
} jc_dev_interface_t;

typedef struct {
	jc_dev_interface_t base;
	joycon_t *jc;
	int jc_revents;
	int ui_revents;
} jc_device_t;

typedef struct {
	jc_dev_interface_t base;
	joycon_t *left;
	joycon_t *right;
	int left_revents;
	int right_revents;
	int ui_revents;
} djc_device_t;

jc_device_t *jd_device_from_jc(joycon_t *jc);
djc_device_t *jd_device_from_jc2(joycon_t *left, joycon_t *right);
int jd_post_events_single(jc_device_t *dev);
int jd_post_events_duo(djc_device_t *dev);
int jd_wait_readable(int n, jc_dev_interface_t **devs);
int next_tick();

#endif /* JOYCON_H */