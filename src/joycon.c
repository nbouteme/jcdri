#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include <joycon.h>
#include "utils.h"

void print_small_report(std_small_report_t rep) {
	printf("Buttons: 0x%04x\n", rep.buttons);
	printf("Stick: 0x%02x\n", rep.stick);
}

void dump_data(void *data, int size) {
	unsigned char *d = data;
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

void print_full_report(std_input_report_t in) {
	std_full_report_t rep = in.full;
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

void print_input_report(std_input_report_t rep) {
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

int jc_send_rcmd(joycon_t *jc, std_output_report_t *cmd) {
	if (hid_write(jc->dev, (void*)cmd, sizeof(*cmd)) < 0) {
		dev_error(jc->dev, "Failed to write");
		return 0;
	}
	return 1;
}

int next_tick() {
	static int pn = 0;
	return pn++;
}

int jc_read_message(joycon_t *jc, std_input_report_t *res) {
	if (hid_read(jc->dev, (void*)res, sizeof(*res)) < 0) {
		dev_error(jc->dev, "Failed to write");
		return 0;
	}
	return 1;
}

int jc_send_cmd(joycon_t *jc, std_output_report_t *cmd) {
	memcpy(cmd->cmd.rumble_data, jc->rumble_data, 8);
	cmd->cmd.timer = next_tick();
	return jc_send_rcmd(jc, cmd);
}

int jc_send_rumble(joycon_t *jc, void *data) {
	std_output_report_t cmd;
	cmd.id = 0x10;
	cmd.cmd.timer = next_tick();
	memcpy(cmd.cmd.rumble_data, data, 8);
	return jc_send_rcmd(jc, &cmd);
}

void jc_set_rumble(joycon_t *jc, int frequency, int intensity) {
	std_output_report_t cmd;
	cmd.id = 0x10;
	memset(&cmd, 0, sizeof(cmd));

	cmd.cmd.rumble_data[intensity] = 1;
	cmd.cmd.rumble_data[1 + 4 + intensity] = 1;

	if (jc->type == LEFT) {
		cmd.cmd.rumble_data[0] = frequency;// (0, 255)
	} else {
		cmd.cmd.rumble_data[4] = frequency;// (0, 255)
	}

	memcpy(jc->rumble_data, cmd.cmd.rumble_data, 8);
	jc_send_rcmd(jc, &cmd);
}

int jc_get_joycons(joycon_t *buf, int s) {
	struct hid_device_info *hids = hid_enumerate(0x057e, 0);
	struct hid_device_info *begin = hids;
	int n = 0;
	while (begin) {
		if (begin->product_id >> 4 == 0x200)
			++n;
		begin = begin->next;
	}

	if (buf) {
		begin = hids;
		while (begin && s) {
			memset(buf, 0, sizeof(*buf));
			buf->type = begin->product_id == 0x2006 ? LEFT : RIGHT;
			buf->dev = hid_open(begin->vendor_id, begin->product_id, begin->serial_number);
			buf->serial = strdup((void*)begin->serial_number);
			++buf;
			--s;
			begin = begin->next;
		}
	}
	hid_free_enumeration(hids);
	return n;
}

joycon_t *jc_get_joycon(jctype_t type) {
	hid_device *dev = hid_open(0x057e, type == LEFT ? 0x2006 : 0x2007, 0);
	if (!dev)
		return 0;
	joycon_t *ret = malloc(sizeof(*ret));
	memset(ret, 0, sizeof(*ret));
	ret->type = type;
	ret->dev = dev;
	return ret;
}

void jc_set_input_mode(joycon_t *jc, jcinput_t mode) {	
	std_output_report_t cmd;
	std_input_report_t res;

	cmd.id = 1;
	cmd.cmd.sub_cmd = 3;
	cmd.cmd.rargs[0] = mode;
	jc->mode = mode;
	jc_send_cmd(jc, &cmd);
	do {
		jc_read_message(jc, &res);
	} while (res.id != 0x21 || res.full.reply.reply_to != 0x03);
}

void jc_poll(joycon_t *jc) {
	std_input_report_t res;

	if (jc->mode != FULL)
		jc_set_input_mode(jc, FULL);	
	do {
		jc_read_message(jc, &res);
	} while (res.id != 0x30);
	jc->connection = res.full.ci;
	jc->battery = res.full.bs;
	jc->rbuttons = res.full.buttons;

	int sh, sv;
	if (jc->type == LEFT) {
		sh = res.full.left_stick_x;
		sv = res.full.left_stick_y;
	} else {
		sh = res.full.right_stick_x;
		sv = res.full.right_stick_y;
	}

	jc->rstick[0] = sh;
	jc->rstick[1] = sv;

	jc->acc_comp = res.full.imu_samples.frames[0].acc;
	jc->gyro_comp = res.full.imu_samples.frames[0].gyro;
}

int jc_get_player_leds(joycon_t *jc) {
	std_output_report_t cmd;
	std_input_report_t res;

	cmd.id = 1;
	cmd.cmd.sub_cmd = 0x31;
	jc_send_cmd(jc, &cmd);
	do {
		jc_read_message(jc, &res);
	} while (res.id != 0x21 || res.full.reply.reply_to != cmd.cmd.sub_cmd);
	return res.full.reply.data[0];
}

void jc_set_player_leds(joycon_t *jc, uint8_t val) {
	std_output_report_t cmd;
	std_input_report_t res;

	cmd.id = 1;
	cmd.cmd.sub_cmd = 0x30;
	cmd.cmd.rargs[0] = val;
	jc_send_cmd(jc, &cmd);
	do {
		jc_read_message(jc, &res);
	} while (res.id != 0x21 || res.full.reply.reply_to != cmd.cmd.sub_cmd);
}

void jc_set_home_led(joycon_t *jc, uint8_t val) {
	std_output_report_t cmd;
	std_input_report_t res;

	cmd.id = 1;
	cmd.cmd.sub_cmd = 0x38;
	memset(cmd.cmd.rargs, 0, sizeof(cmd.cmd.rargs));
	if (jc->type == LEFT)
		return;
	if (val) {
		memset(cmd.cmd.rargs, 0xFF, 20);
		cmd.cmd.rargs[0] = 0x0F;
		cmd.cmd.rargs[1] = 0xF0;
	}
	jc_send_cmd(jc, &cmd);
	do {
		jc_read_message(jc, &res);
	} while (res.id != 0x21 || res.full.reply.reply_to != cmd.cmd.sub_cmd);
}

void jc_set_imu(joycon_t *jc, uint8_t val) {
	std_output_report_t cmd;
	std_input_report_t res;
	val = !!val;
	jc->imu_enabled = val;

	cmd.id = 1;
	cmd.cmd.sub_cmd = 0x40;
	cmd.cmd.rargs[0] = val;
	jc_send_cmd(jc, &cmd);
	do {
		jc_read_message(jc, &res);
	} while (res.id != 0x21 || res.full.reply.reply_to != cmd.cmd.sub_cmd);
}

void jc_set_vibration(joycon_t *jc, uint8_t val) {
	std_output_report_t cmd;
	std_input_report_t res;

	val = !!val;
	cmd.id = 1;
	cmd.cmd.sub_cmd = 0x48;
	cmd.cmd.rargs[0] = val;
	jc_send_cmd(jc, &cmd);
	do {
		jc_read_message(jc, &res);
	} while (res.id != 0x21 || res.full.reply.reply_to != cmd.cmd.sub_cmd);
}

void jc_read_calibration_data(joycon_t *jc) {
	std_output_report_t cmd;
	std_input_report_t res;

	//unsigned char o[64] = {0};
	//uint32_t *p = (void*)o;

	// calibrage stick gauche
	cmd.id = 0x1;
	cmd.cmd.sub_cmd = 0x10;
	cmd.cmd.spi_read_args.size = 9;
	if (jc->type == LEFT) {
		cmd.cmd.spi_read_args.addr = 0x603D;
		jc_send_cmd(jc, &cmd);
		do {
			jc_read_message(jc, &res);
		} while (res.id != 0x21 || res.full.reply.reply_to != cmd.cmd.sub_cmd);
		memcpy(&jc->lcal, res.full.reply.data + 5, sizeof(jc->lcal));
		jc->lcal.max_x += jc->lcal.cen_x;
		jc->lcal.max_y += jc->lcal.cen_y;
		jc->lcal.min_x  = jc->lcal.cen_x - jc->lcal.min_x;
		jc->lcal.min_y  = jc->lcal.cen_y - jc->lcal.min_y;
	} else {
		cmd.cmd.spi_read_args.addr = 0x6046;
		jc_send_cmd(jc, &cmd);
		do {
			jc_read_message(jc, &res);
		} while (res.id != 0x21 || res.full.reply.reply_to != cmd.cmd.sub_cmd);
		memcpy(&jc->rcal, res.full.reply.data + 5, sizeof(jc->rcal));
		jc->rcal.max_x += jc->rcal.cen_x;
		jc->rcal.max_y += jc->rcal.cen_y;
		jc->rcal.min_x  = jc->rcal.cen_x - jc->rcal.min_x;
		jc->rcal.min_y  = jc->rcal.cen_y - jc->rcal.min_y;
	}

	// parametres sticks
	cmd.cmd.spi_read_args.addr = 0x6086;
	cmd.cmd.spi_read_args.size = 18;
	jc_send_cmd(jc, &cmd);
	do {
		jc_read_message(jc, &res);
	} while (res.id != 0x21 || res.full.reply.reply_to != cmd.cmd.sub_cmd);
	memcpy(&jc->stick_prop, res.full.reply.data + 8, 6);

	// IMU (parametres)
	cmd.cmd.spi_read_args.addr = 0x6020;
	cmd.cmd.spi_read_args.size = 0x18;
	jc_send_cmd(jc, &cmd);
	do {
		jc_read_message(jc, &res);
	} while (res.id != 0x21 || res.full.reply.reply_to != cmd.cmd.sub_cmd);
	memcpy(&jc->acc_orig, res.full.reply.data + 5, 12);
}

void jc_print_joycon(joycon_t *left) {
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
	printf("Name: %s\n", left->type == LEFT ? "Joycon L" : "Joycon R");
	printf("Battery: %s\n", battery_str[left->battery]);
	printf("Deadzone Range: [%5u, %5u]\n", left->stick_prop.deadzone, left->stick_prop.range);
	printf("Raw Stick [%5u %5u]\n", left->rstick[0], left->rstick[1]);
	if (left->type == LEFT) {
		printf("Max XY [%5u %5u]\n", left->lcal.max_x, left->lcal.max_y);
		printf("Center XY [%5u %5u]\n", left->lcal.cen_x, left->lcal.cen_y);
		printf("Min XY [%5u %5u]\n", left->lcal.min_x, left->lcal.min_y);
	} else {
		printf("Max XY [%5u %5u]\n", left->rcal.max_x, left->rcal.max_y);
		printf("Center XY [%5u %5u]\n", left->rcal.cen_x, left->rcal.cen_y);
		printf("Min XY [%5u %5u]\n", left->rcal.min_x, left->rcal.min_y);
	}
	if (left->type == LEFT) {
		printf("up: %9s down: %9s left: %9s right: %9s\n",
			   left->up ? "pressed" : "released",
			   left->down ? "pressed" : "released",
			   left->left ? "pressed" : "released",
			   left->right ? "pressed" : "released");
		printf("L: %9s ZL: %9s Left Stick Button: %9s\n",
			   left->l ? "pressed" : "released",
			   left->zl ? "pressed" : "released",
			   left->ls ? "pressed" : "released");
		printf("Capture: %9s Minus: %9s\n",
			   left->capture ? "pressed" : "released",
			   left->minus ? "pressed" : "released");
		printf("SL: %9s SR: %9s\n",
			   left->lsl ? "pressed" : "released",
			   left->lsr ? "pressed" : "released");
	} else {
		printf("A: %9s B: %9s X: %9s Y: %9s\n",
			   left->a ? "pressed" : "released",
			   left->b ? "pressed" : "released",
			   left->x ? "pressed" : "released",
			   left->y ? "pressed" : "released");
		printf("R: %9s ZR: %9s Right Stick Button: %9s\n",
			   left->r ? "pressed" : "released",
			   left->zr ? "pressed" : "released",
			   left->rs ? "pressed" : "released");
		printf("Home: %9s Plus: %9s\n",
			   left->home ? "pressed" : "released",
			   left->plus ? "pressed" : "released");
		printf("SL: %9s SR: %9s\n",
			   left->rsl ? "pressed" : "released",
			   left->rsr ? "pressed" : "released");
	}
	puts("\n\n");
}
