#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/poll.h>

#include "joycon.h"
#include "rumble.h"

void upload_effect(jc_dev_interface_t *dev, struct ff_effect effect) {
	int idx = effect.id;
	if ((dev->used_patterns >> idx) & 1)
		return;
	dev->rumble_patterns[idx] = effect;
	dev->used_patterns |= 1 << idx;
}

void erase_effect(jc_dev_interface_t *dev, int effect) {
	int idx = effect;
	if (!((dev->used_patterns >> idx) & 1))
		return;
	dev->used_patterns ^= 1 << idx;
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
		case UI_FF_UPLOAD:;
			struct uinput_ff_upload upload;
			memset(&upload, 0, sizeof(upload));
			upload.request_id = ev.value;
			ioctl(fd, UI_BEGIN_FF_UPLOAD, &upload);
			upload_effect(dev, upload.effect);
			upload.retval = 0;
			ioctl(fd, UI_END_FF_UPLOAD, &upload);
			break;
		case UI_FF_ERASE:;
			struct uinput_ff_erase erase;
			memset(&erase, 0, sizeof(erase));
			erase.request_id = ev.value;
			ioctl(fd, UI_BEGIN_FF_ERASE, &erase);
			erase_effect(dev, erase.effect_id);
			erase.retval = 0;
			ioctl(fd, UI_END_FF_ERASE, &erase);
			break;
		}
		break;
	}
}

int jd_wait_readable(int n, jc_dev_interface_t **devs) {
	jc_dev_interface_t *reordered[n];
	memset(reordered, 0, sizeof(reordered));
	int idx = 0;
	for (int i = 0; i < n; ++i) {
		if (devs[i])
			reordered[idx++] = devs[i];
	}

	struct pollfd fds[idx << 2];
	int nfds = 0;
	for(int i = 0; i < idx; ++i) {
		nfds += reordered[i]->write_fds(reordered[i], fds + nfds);
	}
	poll(fds, nfds, -1);
	nfds = 0;
	for(int i = 0; i < idx; ++i) {
		nfds += reordered[i]->read_fds(reordered[i], fds + nfds);
	}
	memcpy(devs, reordered, sizeof(reordered));
	return idx;
}
