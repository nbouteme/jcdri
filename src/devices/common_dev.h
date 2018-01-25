#ifndef COMMON_DEV_H
#define COMMON_DEV_H

#include <joycon.h>

void upload_effect(jc_dev_interface_t *dev, struct ff_effect effect);
void erase_effect(jc_dev_interface_t *dev, int effect);
void handle_input_events(jc_dev_interface_t *dev);
void run_events_single(jc_dev_interface_t *dev);

#endif /* COMMON_DEV_H */