#ifndef BLUETOOTH_HID_H
#define BLUETOOTH_HID_H

#include <stdint.h>
#define PACKED __attribute__((packed))

typedef unsigned uint;

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

#endif /* BLUETOOTH_HID_H */