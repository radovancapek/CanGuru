#include "canguru-device.h"

#include "canguru-net.h"

static void canguru_dev_deinit(struct canguru_device *self);

int canguru_dev_init(struct canguru_device *self,
					 struct usb_interface *intf,
					 struct class *guru_class)
{
	struct canguru_channel_conf net_conf;
	const struct canguru_msg_net_conf msg_conf = {.guru_dev = &self->guru_dev,
												  .netdev = self->netdev,
												  .iface_count = CANGURU_CHANNEL_COUNT};
	const struct guru_boot_conf boot_conf = {.guru_dev = &self->guru_dev};
	int idx;
	int err;

	self->guru_dev.dev_data = self;
	self->guru_dev.dev_deinit = (void (*)(void *))canguru_dev_deinit;

	for (idx = 0; idx < CANGURU_CHANNEL_COUNT; idx++) {
		self->netdev[idx] = NULL;
	}

	err = guru_dev_init(&self->guru_dev, intf, guru_class);
	if (err != 0) {
		return err;
	}

	net_conf.guru_dev = &self->guru_dev;
	net_conf.msg = &self->can_msg_net;

	for (idx = 0; idx < CANGURU_CHANNEL_COUNT; idx++) {
		net_conf.channel_idx = idx;
		canguru_net_create(&self->netdev[idx], &net_conf);
	}

	err = canguru_msg_net_init(&self->can_msg_net, &msg_conf);
	if (err != 0) {
		return err;
	}

	err = guru_boot_create(&self->boot, &boot_conf);
	if (err != 0) {
		return err;
	}

	return 0;
}

static void canguru_dev_deinit(struct canguru_device *self)
{
	int idx;

	guru_boot_destroy(&self->boot);
	
	for (idx = 0; idx < CANGURU_CHANNEL_COUNT; idx++) {
		canguru_net_destroy(&self->netdev[idx]);
	}
}
