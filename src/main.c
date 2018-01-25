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

#include <joycon.h>
#include <rumble.h>

#include "devices/single_dev.h"
#include "devices/dual_dev.h"

void setup_new_jc(joycon_t *jc) {
	jc_set_player_leds(jc, 0xF0);
	jc_set_imu(jc, 1);
	jc_set_input_mode(jc, STD);	
	jc_set_vibration(jc, 0);
	jc_read_calibration_data(jc);
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
		{ .buttons = { .left = 1 }},
		{ .buttons = { .a    = 1 }}
	};
	union btn_u cancel_mask[2] = {
		{ .buttons = { .down = 1 }},
		{ .buttons = { .b    = 1 }}
	};

	if (!*jc) {
		*jc = jc_get_joycon(type);
		if (*jc) {
			setup_new_jc(*jc);
		}
		return;
	}
	jc_poll(*jc);
	if (dsvalid)
		return;
	if (!*isvalid && ((*jc)->rbuttons & srsl_mask[type].rbuttons) == srsl_mask[type].rbuttons) {
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

int main()
{
	joycon_t *left = 0;
	joycon_t *right = 0;
	char lsvalid = 0;
	char rsvalid = 0;
	char dsvalid = 0;

	uint8_t rb[9] = {0};
	make_rumble_data(320, 160, 0.5, 0.5, rb + 1);
//	joycon_t *known_jcs = 0;
//	int nknown_jcs = 0;
	while (1) {
		int jc_get_joycons(joycon_t *buf, int s);

		int njcs = jc_get_joycons(0, 0);
		joycon_t *buf = malloc(sizeof(joycon_t[njcs]));

		for (int i = 0; i < njcs; ++i) {
		}
		
		
		free(buf);
		/*
		  Recuperer les joycons
		  Souvenir de ceux qui ne le sont pas déjà
		  poll inputs
		  ceux qui sont sr+sl -> ajouté aux association singles
		  ceux qui sont l -> ajouté au association pairs
		  ceux qui sont r -> ajouté aux association pairs
		  si pair l/r complete, ajouter pair à association dual
		  si b/down, retirer de l'association
		  si déconnecté, retirer des souvenirs
		  si tout est associé et validé, sortie
		 */
		try_pair_joycon(LEFT, &left, &lsvalid, dsvalid);
		try_pair_joycon(RIGHT, &right, &rsvalid, dsvalid);

		/* TODO: Handle disconnection */
		if (lsvalid == 2 && rsvalid == 2)
			break;
		if (lsvalid == 2 && !right)
			break;
		if (rsvalid == 2 && !left)
			break;

		if (left && right && !lsvalid && !rsvalid) {
			if (!dsvalid && left->l && right->r) {
				rb[0] = next_tick();
				jc_set_player_leds(left, 0x10);
				jc_set_player_leds(right, 0x10);
				memcpy(left->rumble_data, rb + 1, 8);
				memcpy(right->rumble_data, rb + 1, 8);
				jc_set_vibration(left, 1);
				jc_set_vibration(right, 1);
				usleep(10);
				jc_set_vibration(left, 0);
				jc_set_vibration(right, 0);
				
				dsvalid = 1;
			}
			if (dsvalid == 1) {
				if (right->a) {
					jc_set_player_leds(left, 0x01);
					jc_set_player_leds(right, 0x01);
					memcpy(left->rumble_data, rb + 1, 8);
					memcpy(right->rumble_data, rb + 1, 8);
					jc_set_vibration(left, 1);
					jc_set_vibration(right, 1);
					usleep(10);
					jc_set_vibration(left, 0);
					jc_set_vibration(right, 0);

					dsvalid = 2;
				}
				if (right->b) {
					jc_set_player_leds(left, 0xF0);
					jc_set_player_leds(right, 0xF0);
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

	jc_dev_interface_t *ldev = 0;
	jc_dev_interface_t *rdev = 0;
	jc_dev_interface_t *ddev = 0;

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

	jc_dev_interface_t *devs[3] = {ldev, rdev, ddev};

	while (1) {
		int n = jd_wait_readable(3, devs);
		for (int i = 0; i < n; ++i) {
			devs[i]->run_events(devs[i]);
		}
	}
    return 0;
}
