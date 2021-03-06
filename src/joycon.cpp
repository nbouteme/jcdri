#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <event_loop.h>

#include "udev.h"
#include <joycon.h>
#include "utils.h"

namespace jcdri {

	void print_small_report(std_small_report rep) {
		printf("Buttons: 0x%04x\n", rep.buttons);
		printf("Stick: 0x%02x\n", rep.stick);
	}

	void dump_data(void *data, int size) {
		unsigned char *d = (decltype(d))data;
		int n = 0;
		int i = 0;
		while (size--) {
			printf("%02x ", d[i++]);
			n++;
			if (n == 8) {
				n = 0;
				printf("\n");
			}
		}
		if (n)
			printf("\n");
	}

	void print_full_report(std_input_report in) {
		std_full_report rep = in.full;
		printf("Timer: 0x%04x\n", rep.timer);
		printf("BS/CI: 0x%02x\n", rep.bs << 2 | rep.ci);
		printf("Buttons: 0x%06x\n", rep.buttons);
		printf("Left Stick: 0x%06x\n", rep.left_stick_x << 12 | rep.left_stick_y);
		printf("Right Stick: 0x%06x\n", rep.right_stick_x << 12 | rep.right_stick_y);
		printf("Vibration: 0x%02x\n", rep.vibrator_report);
		switch(in.id) {
		case 0x21:
			printf("Ack: 0x%02x\n", rep.reply.ack);
			printf("Reply To: 0x%02x\n", rep.reply.reply_to);
			printf("Data : \n");
			dump_data(rep.reply.data, 35);
			break;
		case 0x30:
			dump_data(&rep.imu_samples, sizeof(rep.imu_samples));
		default:
			break;
		}
	
	}

	void print_input_report(std_input_report rep) {
		printf("ID: 0x%04x\n", rep.id);
		switch (rep.id) {
		case 0x3F:
			print_small_report(rep.std);
			break;
		default:
			print_full_report(rep);
			break;
		}
		puts("\n");
	}

	int joycon::send_rcmd(std_output_report &cmd) {
		if (hid_write(dev, (const unsigned char*)&cmd, sizeof(cmd)) < 0) {
			dev_error(dev, "Failed to write");
			return 0;
		}
		return 1;
	}

	int next_tick() {
		static int pn = 0;
		return pn++;
	}

	int joycon::read_message(std_input_report &res, int assert_size = -1) {
		int n;
		// TODO: make a message queue
		if ((n = hid_read(dev, (unsigned char*)&res, sizeof(res))) < 0) {
			dev_error(dev, "Failed to write");
			return 0;
		}
		if (assert_size >= 0 && n != assert_size) {
			printf("Expected %d, got %d bytes\n", assert_size, n);
			abort();
		}
		return 1;
	}

	int joycon::send_cmd(std_output_report &cmd) {
		memcpy(cmd.cmd.rumble_data, rumble_data, 8);
		cmd.cmd.timer = next_tick();
		return send_rcmd(cmd);
	}

	int joycon::send_rumble(unsigned char *data) {
		std_output_report cmd;

		memset(&cmd, 0, sizeof(cmd));
		cmd.id = 0x10;
		cmd.cmd.timer = next_tick();
		memcpy(cmd.cmd.rumble_data, data, 8);
		return send_rcmd(cmd);
	}

	int joycon::set_rumble(int frequency, int intensity) {
		std_output_report cmd;

		memset(&cmd, 0, sizeof(cmd));
		cmd.id = 0x10;
		cmd.cmd.rumble_data[intensity] = 1;
		cmd.cmd.rumble_data[1 + 4 + intensity] = 1;

		if (type == LEFT) {
			cmd.cmd.rumble_data[0] = frequency;// (0, 255)
		} else {
			cmd.cmd.rumble_data[4] = frequency;// (0, 255)
		}

		memcpy(rumble_data, cmd.cmd.rumble_data, 8);
		send_rcmd(cmd);
		return 1;
	}

	void joycon::set_mode(jcinput mode) {	
		std_output_report cmd;

		memset(&cmd, 0, sizeof(cmd));
		cmd.id = 1;
		cmd.cmd.sub_cmd = 3;
		cmd.cmd.rargs[0] = mode;
		this->mode = mode;
		sync_send_cmd(cmd);
	}

	void joycon::poll() {
		std_input_report res;

		if (mode != FULL)
			set_mode(FULL);	
		do {
			if (!read_message(res))
				return;
		} while (res.id != 0x30);
		connection = res.full.ci;
		battery = res.full.bs;
		rbuttons = res.full.buttons;

		int sh, sv;
		if (type == LEFT) {
			sh = res.full.left_stick_x;
			sv = res.full.left_stick_y;
		} else {
			sh = res.full.right_stick_x;
			sv = res.full.right_stick_y;
		}

		rstick[0] = sh;
		rstick[1] = sv;

		acc_comp = res.full.imu_samples.frames[0].acc;
		gyro_comp = res.full.imu_samples.frames[0].gyro;
		if (jdev)
			jdev->post_events();
	}

	int joycon::get_player_leds() {
		std_output_report cmd;
		std_input_report res;

		cmd.id = 1;
		cmd.cmd.sub_cmd = 0x31;
		res = sync_send_cmd(cmd);
		return res.full.reply.data[0];
	}

	void joycon::set_player_leds(uint8_t val) {
		std_output_report cmd;

		memset(&cmd, 0, sizeof(cmd));
		cmd.id = 1;
		cmd.cmd.sub_cmd = 0x30;
		cmd.cmd.rargs[0] = val;
		send_cmd(cmd);
		sync_send_cmd(cmd);
	}

	void joycon::set_home_led(uint8_t val) {
		std_output_report cmd;

		cmd.id = 1;
		cmd.cmd.sub_cmd = 0x38;
		memset(cmd.cmd.rargs, 0, sizeof(cmd.cmd.rargs));
		if (type == LEFT)
			return;
		if (val) {
			memset(cmd.cmd.rargs, 0xFF, 20);
			cmd.cmd.rargs[0] = 0x0F;
			cmd.cmd.rargs[1] = 0xF0;
		}
		sync_send_cmd(cmd);
	}

	std_input_report joycon::sync_send_cmd(std_output_report &cmd) {
		std_input_report res;
		send_cmd(cmd);
		do {
			read_message(res);
		} while (res.id != 0x21);
		if (res.full.reply.reply_to != cmd.cmd.sub_cmd)
			abort();
		return res;
	}

	void joycon::set_imu(uint8_t val) {
		std_output_report cmd;
		val = !!val;
		imu_enabled = val;

		memset(&cmd, 0, sizeof(cmd));
		cmd.id = 1;
		cmd.cmd.sub_cmd = 0x40;
		cmd.cmd.rargs[0] = val;
		sync_send_cmd(cmd);
	}

	void joycon::set_vibration(uint8_t val) {
		std_output_report cmd;

		val = !!val;
		memset(&cmd, 0, sizeof(cmd));
		cmd.id = 1;
		cmd.cmd.sub_cmd = 0x48;
		cmd.cmd.rargs[0] = val;
		sync_send_cmd(cmd);
	}

	void joycon::read_calibration_data() {
		std_output_report cmd;
		std_input_report res;

		//unsigned char o[64] = {0};
		//uint32_t *p = (void*)o;

		// calibrage stick gauche
		memset(&cmd, 0, sizeof(cmd));
		cmd.id = 0x1;
		cmd.cmd.sub_cmd = 0x10;
		cmd.cmd.sra.size = 9;
		if (type == LEFT) {
			cmd.cmd.sra.addr = 0x603D;
			res = sync_send_cmd(cmd);
			memcpy(&lcal, res.full.reply.data + 5, sizeof(lcal));
			lcal.max_x += lcal.cen_x;
			lcal.max_y += lcal.cen_y;
			lcal.min_x  = lcal.cen_x - lcal.min_x;
			lcal.min_y  = lcal.cen_y - lcal.min_y;
		} else {
			cmd.cmd.sra.addr = 0x6046;
			res = sync_send_cmd(cmd);
			memcpy(&rcal, res.full.reply.data + 5, sizeof(rcal));
			rcal.max_x += rcal.cen_x;
			rcal.max_y += rcal.cen_y;
			rcal.min_x  = rcal.cen_x - rcal.min_x;
			rcal.min_y  = rcal.cen_y - rcal.min_y;
		}

		// parametres sticks
		cmd.cmd.sra.addr = 0x6086;
		cmd.cmd.sra.size = 18;
		res = sync_send_cmd(cmd);
		memcpy(&stick_prop, res.full.reply.data + 8, 6);

		// IMU (parametres)
		cmd.cmd.sra.addr = 0x6020;
		cmd.cmd.sra.size = 0x18;
		res = sync_send_cmd(cmd);
		memcpy(&acc_orig, res.full.reply.data + 5, 12);
	}

	void joycon::print() {
		const char *battery_str[] = {
			"Empty",
			"Critical",
			"Critical",
			"Low",
			"Low",
			"Mid",
			"Mid",
			"Full",
			"Full"
		};
		printf("Name: %s\n", type == LEFT ? "Joycon L" : "Joycon R");
		printf("Battery: %s\n", battery_str[battery]);
		printf("Deadzone Range: [%5u, %5u]\n", stick_prop.deadzone, stick_prop.range);
		printf("Raw Stick [%5u %5u]\n", rstick[0], rstick[1]);
		printf("Gyroscope [%5d, %5d, %5d]\n", gyro_comp.x, gyro_comp.y, gyro_comp.z);
		printf("Accelerometre [%5d, %5d, %5d]\n", acc_comp.x, acc_comp.y, acc_comp.z);
		if (type == LEFT) {
			printf("Max XY [%5u %5u]\n", lcal.max_x, lcal.max_y);
			printf("Center XY [%5u %5u]\n", lcal.cen_x, lcal.cen_y);
			printf("Min XY [%5u %5u]\n", lcal.min_x, lcal.min_y);
		} else {
			printf("Max XY [%5u %5u]\n", rcal.max_x, rcal.max_y);
			printf("Center XY [%5u %5u]\n", rcal.cen_x, rcal.cen_y);
			printf("Min XY [%5u %5u]\n", rcal.min_x, rcal.min_y);
		}
		if (type == LEFT) {
			printf("up: %9s down: %9s left: %9s right: %9s\n",
				   up ? "pressed" : "released",
				   down ? "pressed" : "released",
				   left ? "pressed" : "released",
				   right ? "pressed" : "released");
			printf("L: %9s ZL: %9s Left Stick Button: %9s\n",
				   l ? "pressed" : "released",
				   zl ? "pressed" : "released",
				   ls ? "pressed" : "released");
			printf("Capture: %9s Minus: %9s\n",
				   capture ? "pressed" : "released",
				   minus ? "pressed" : "released");
			printf("SL: %9s SR: %9s\n",
				   lsl ? "pressed" : "released",
				   lsr ? "pressed" : "released");
		} else {
			printf("A: %9s B: %9s X: %9s Y: %9s\n",
				   a ? "pressed" : "released",
				   b ? "pressed" : "released",
				   x ? "pressed" : "released",
				   y ? "pressed" : "released");
			printf("R: %9s ZR: %9s Right Stick Button: %9s\n",
				   r ? "pressed" : "released",
				   zr ? "pressed" : "released",
				   rs ? "pressed" : "released");
			printf("Home: %9s Plus: %9s\n",
				   home ? "pressed" : "released",
				   plus ? "pressed" : "released");
			printf("SL: %9s SR: %9s\n",
				   rsl ? "pressed" : "released",
				   rsr ? "pressed" : "released");
		}
		puts("\n\n");
	}

	joycon::joycon(hid_device_info *info) {
		dev = hid_open_path(info->path);
		serial = strdup((char*)info->serial_number);
		if(info->product_id == 0x2006)
			type = LEFT;
		else
			type = RIGHT;
	}

	joycon::joycon(libudev::device &hidraw, const std::string &s) {
		dev = hid_open_path(hidraw.get_devnode().c_str());
		serial = strdup(s.c_str());
		unsigned bus, vendor, product;
		sscanf(s.c_str(), "%x:%x:%x", &bus, &vendor, &product);
		if(product == 0x2006)
			type = LEFT;
		else
			type = RIGHT;
	}

	joycon::~joycon() {
		puts("destructing joycon");
		if (jdev)
			jdev->el->remove_source(jdev);
	}
}