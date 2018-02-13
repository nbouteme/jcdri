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

namespace jcdri {
	void jc_device::upload_effect(struct ff_effect effect) {
		int idx = effect.id;
		if ((used_patterns >> idx) & 1)
			return;
		rumble_patterns[idx] = effect;
		used_patterns |= 1 << idx;
	}

	void jc_device::erase_effect(int effect) {
		int idx = effect;
		if (!((used_patterns >> idx) & 1))
			return;
		used_patterns ^= 1 << idx;
	}

	void jc_device::handle_input_events() {
		int fd = uifd;
		struct input_event ev;
		read(fd, &ev, sizeof(ev));
		switch(ev.type) {
		case EV_FF:
			if (ev.code == FF_GAIN)
				return;
			if (ev.value)
				play_effect(ev.code);
			else
				stop_effect(ev.code);
			break;
		case EV_UINPUT:
			switch (ev.code) {
			case UI_FF_UPLOAD:;
				struct uinput_ff_upload upload;
				memset(&upload, 0, sizeof(upload));
				upload.request_id = ev.value;
				ioctl(fd, UI_BEGIN_FF_UPLOAD, &upload);
				upload_effect(upload.effect);
				upload.retval = 0;
				ioctl(fd, UI_END_FF_UPLOAD, &upload);
				break;
			case UI_FF_ERASE:;
				struct uinput_ff_erase erase;
				memset(&erase, 0, sizeof(erase));
				erase.request_id = ev.value;
				ioctl(fd, UI_BEGIN_FF_ERASE, &erase);
				erase_effect(erase.effect_id);
				erase.retval = 0;
				ioctl(fd, UI_END_FF_ERASE, &erase);
				break;
			}
			break;
		}
	}


	jc_device::jc_device(event_loop *e) : source(e) {
	}

	jc_device::~jc_device() {
		puts("destructing jcdevice");
		libevdev_uinput_destroy(uidev);
		libevdev_free(dev);
	}
}