#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <string.h>
#include <math.h>
#include <pthread.h>
#include <sys/poll.h>

#include "joycon.h"
#include "rumble.h"

#define assert_null(x) {												\
		int __error_code = x;											\
		if (__error_code) {												\
			fprintf(stderr, "Failed at %s:%d\n", __FILE__, __LINE__);	\
			return 1;													\
		}																\
	}																	\

void setup_new_jc(joycon_t *jc) {
	jc_set_player_leds(jc, 0xF0);
	jc_set_imu(jc, 1);
	if (jc->imu_enabled == 0) {
		printf("%s:%d\n", __FILE__, __LINE__);
		abort();
	}
	jc_set_input_mode(jc, STD);	
	if (jc->imu_enabled == 0) {
		printf("%s:%d\n", __FILE__, __LINE__);
		abort();
	}
	jc_set_vibration(jc, 0);
	if (jc->imu_enabled == 0) {
		printf("%s:%d\n", __FILE__, __LINE__);
		abort();
	}
	jc_read_calibration_data(jc);
	if (jc->imu_enabled == 0) {
		printf("%s:%d\n", __FILE__, __LINE__);
		abort();
	}
}

void try_pair_joycon(jctype_t type, joycon_t **jc, char *isvalid, char dsvalid) {
	union btn_u {
		named_buttons_t buttons;
		int rbuttons;
	};
	union btn_u srsl_mask[2] = {
		{ .buttons = { .lsl = 1, .lsr = 1 }},
		{ .buttons = { .rsl = 1, .rsr = 1 }}
	};
	union btn_u valid_mask[2] = {
		{ .buttons = { .down = 1 }},
		{ .buttons = { .a    = 1 }}
	};
	union btn_u cancel_mask[2] = {
		{ .buttons = { .left = 1 }},
		{ .buttons = { .b    = 1 }}
	};

	if (dsvalid)
		return;
	if (!*jc) {
		*jc = jc_get_joycon(type);
		if (*jc) {
			setup_new_jc(*jc);
		}
		return;
	}
	jc_poll(*jc);
	if (!*isvalid && ((*jc)->rbuttons & srsl_mask[type].rbuttons) == srsl_mask[type].rbuttons) {
		jc_print_joycon(*jc);
		play_freq(*jc, 320, 160, 0.5f, 0.5f, 100000);
		jc_set_player_leds(*jc, 0x10);
		*isvalid = 1;
	}
	if (*isvalid && (*jc)->rbuttons & valid_mask[type].rbuttons) {
		play_freq(*jc, 320, 160, 0.5f, 0.5f, 100000);
		jc_set_player_leds(*jc, 0x01 + type);
		*isvalid = 2;
	}
	if ((*jc)->rbuttons & cancel_mask[type].rbuttons) {
		jc_set_player_leds(*jc, 0xF0);
		*isvalid = 0;
	}
}

typedef struct {
	jc_device_t *left;
	jc_device_t *right;
	djc_device_t *dual;
	char lsvalid;
	char rsvalid;
	char dsvalid;
} aspe_args_t;

void upload_effect(jc_dev_interface_t *dev, struct ff_effect effect) {
	int idx = effect.id;
	puts("Received an effect");
	if ((dev->used_patterns >> idx) & 1)
		return;
	dev->rumble_patterns[idx] = effect;
	dev->used_patterns |= 1 << idx;
}

void erase_effect(jc_dev_interface_t *dev, int effect) {
	puts("Deleting an effect");
	int idx = effect;
	if (!((dev->used_patterns >> idx) & 1))
		return;
	dev->used_patterns ^= 1 << idx;
}

void *async_send_post_events(void *up) {
	aspe_args_t *args = up;
	jc_device_t *ldev = args->left;
	jc_device_t *rdev = args->right;
	djc_device_t *ddev = args->dual;	

	char rsvalid = args->rsvalid;
	char lsvalid = args->lsvalid;
	char dsvalid = args->dsvalid;
	
	free(args);

	while(1) {
		if (dsvalid == 2) {
			jc_poll(ddev->left);
			jc_poll(ddev->right);
			jd_post_events_duo(ddev);
		} else {
			if (lsvalid == 2) {
				jc_poll(ldev->jc);
				jd_post_events_single(ldev);
			}
			if (rsvalid == 2) {
				jc_poll(rdev->jc);
				jd_post_events_single(rdev);
			}
		}
	}

	return 0;
}

void handle_input_events(jc_dev_interface_t *dev) {
	int fd = dev->uifd;
	struct input_event ev;
	read(fd, &ev, sizeof(ev));
	switch(ev.type) {
	case EV_FF:
		if (ev.code == FF_GAIN)
			return;
		if (ev.value)
			dev->play_effect(dev, ev.code);
		else
			dev->stop_effect(dev, ev.code);
		break;
	case EV_UINPUT:
		switch (ev.code) {
		case UI_FF_UPLOAD: {
			struct uinput_ff_upload upload;
			memset(&upload, 0, sizeof(upload));
			upload.request_id = ev.value;
			ioctl(fd, UI_BEGIN_FF_UPLOAD, &upload);
			upload_effect(dev, upload.effect);
			upload.retval = 0;
			ioctl(fd, UI_END_FF_UPLOAD, &upload);
		}
		break;
		case UI_FF_ERASE: {
			struct uinput_ff_erase erase;
			memset(&erase, 0, sizeof(erase));
			erase.request_id = ev.value;
			ioctl(fd, UI_BEGIN_FF_ERASE, &erase);
			erase_effect(dev, erase.effect_id);
			erase.retval = 0;
			ioctl(fd, UI_END_FF_ERASE, &erase);
		}
		break;
		default:
			puts("Received unknown uinput event code");
		}
		break;
	default:
		puts("Received unknown event type");
	}
}



int main()
{
	joycon_t *left = 0;
	joycon_t *right = 0;
	char lsvalid = 0;
	char rsvalid = 0;
	char dsvalid = 0;

	while (1) {
		try_pair_joycon(LEFT, &left, &lsvalid, dsvalid);
		try_pair_joycon(RIGHT, &right, &rsvalid, dsvalid);
		if (left && left->imu_enabled == 0) {
			printf("%s:%d\n", __FILE__, __LINE__);
			abort();
		}

		/* TODO: Handle disconnection */
		if (lsvalid == 2 && rsvalid == 2)
			break;
		if (lsvalid == 2 && !right)
			break;
		if (rsvalid == 2 && !left)
			break;

		if (left && right && !lsvalid && !rsvalid) {
			if (left->buttons.l && right->buttons.r) {
				jc_set_player_leds(left, 0x10);
				jc_set_player_leds(right, 0x10);
				dsvalid = 1;
			}
			if (dsvalid == 1) {
				if (right->buttons.a) {
					jc_set_player_leds(left, 0x01);
					jc_set_player_leds(right, 0x01);
					dsvalid = 2;
				}
				if (right->buttons.b) {
					jc_set_player_leds(left, 0x00);
					jc_set_player_leds(right, 0x00);
					dsvalid = 0;
				}
			}
		}
		if (dsvalid == 2)
			break;
		// Pour valider un joycon simple, il faut faire SL+SR en le
		// tenant horizontalement et valider avec le bouton bas relatif
		// pour annuler il faut appuyer sur le bouton droit relatif

		// Si tout les joycons simples présents ont validé, on peut quitter

		// Pour un joycon double, il faut appuyer sur L et R
		// et valider avec A ou quitter avec B;
	}

	pthread_t t;

	jc_device_t *ldev = 0;
	jc_device_t *rdev = 0;
	djc_device_t *ddev = 0;

	if (left->imu_enabled == 0) {
		printf("%s:%d\n", __FILE__, __LINE__);
		abort();
	}
	if (dsvalid == 2) {
		ddev = jd_device_from_jc2(left, right);
		if (!ddev)
			return 0;
	} else {
		if (rsvalid == 2) {
			rdev = jd_device_from_jc(right);
			if (!rdev) {
				return 0;
			}
		}

		if (lsvalid == 2) {
			ldev = jd_device_from_jc(left);
			if (!ldev) {
				return 0;
			}
		}
	}
	if (left->imu_enabled == 0) {
		printf("%s:%d\n", __FILE__, __LINE__);
		abort();
	}
	aspe_args_t *args = malloc(sizeof(*args));
	*args = (aspe_args_t) {
		ldev, rdev, ddev, lsvalid, rsvalid, dsvalid
	};
	if (left->imu_enabled == 0) {
		printf("%s:%d\n", __FILE__, __LINE__);
		abort();
	}

	pthread_create(&t, 0, async_send_post_events, args);
	jc_dev_interface_t *ass[3] = { &ddev->base, &ldev->base, &rdev->base };
	struct pollfd fds[3];
	jc_dev_interface_t *val[3];
	int idx = 0;
	for (int i = 0; i < 3; ++i) {
		if (ass[i]) {
			val[idx] = ass[i];
			fds[idx++] = (struct pollfd) { ass[i]->uifd, POLLIN, 0 };
		}
	}

	while (1) {
		int n = poll(fds, idx, -1);
		for (int i = 0; i < n; ++i) {
			if (fds[i].revents & POLLIN) {
				handle_input_events(val[i]);
			}
		}
	}
	// unreachable
	//libevdev_uinput_destroy(uidev);
    return 0;
}
