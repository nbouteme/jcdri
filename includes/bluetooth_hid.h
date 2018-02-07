#ifndef BLUETOOTH_HID_H
#define BLUETOOTH_HID_H

#include <stdint.h>
#define PACKED __attribute__((packed))

typedef unsigned uint;

struct subcommand_reply {
	uint8_t ack;
	uint8_t reply_to;
	uint8_t data[35];	
} PACKED;

struct frame_data {
	uint8_t data[38];	
} PACKED;

struct int16vec3 {
	int16_t x, y, z;
} PACKED;

struct imu_frame {
	int16vec3 acc;
	int16vec3 gyro;
} PACKED;

struct full_input {
	imu_frame frames[3];
} PACKED;

struct std_full_report {
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
		subcommand_reply reply;
		full_input imu_samples;
	};
} PACKED;

struct std_small_report {
	uint buttons : 16;
	uint stick : 8;
	uint8_t filler[8];
} PACKED;

struct std_input_report {
	uint8_t id;
	union {
		std_small_report std;
		std_full_report full;
	};
} PACKED;

struct spi_read_args {
	uint32_t addr;
	uint8_t size;
} PACKED;

struct output_cmd {
	uint unused : 4;
	uint timer : 4;
	uint8_t rumble_data[8];
	uint8_t sub_cmd;
	union {
		uint8_t rargs[38];
		spi_read_args sra;
		/* TODO: Add more argument types */
	};
} PACKED;

struct rumble_cmd {
	uint unused : 4;
	uint timer : 4;
	uint8_t rumble_data[8];
} PACKED;

struct std_output_report {
	uint8_t id;
	union {
		output_cmd cmd;
		rumble_cmd rumble;
	};
} PACKED;

#endif /* BLUETOOTH_HID_H */