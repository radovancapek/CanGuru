#include "linguru-device.h"

#include "linguru-net.h"
#include "linguru-voltage.h"

static void linguru_dev_deinit(struct linguru_device *self);

int linguru_dev_init(struct linguru_device *self,
					 struct usb_interface *intf,
					 struct class *guru_class)
{
	struct linguru_channel_conf net_conf;
	struct linguru_msg_net_conf msg_conf = {.guru_dev = &self->guru_dev,
											.netdev = self->netdev,
											.iface_count = LINGURU_CHANNEL_COUNT,
											.meas_volt_dev = NULL};
	const struct linguru_voltage_conf volt_conf = {.guru_dev = &self->guru_dev};
	int idx;
	int err;

	self->guru_dev.dev_data = self;
	self->guru_dev.dev_deinit = (void (*)(void *))linguru_dev_deinit;

	for (idx = 0; idx < LINGURU_CHANNEL_COUNT; idx++) {
		self->netdev[idx] = NULL;
	}
	
	err = guru_dev_init(&self->guru_dev, intf, guru_class);
	if (err != 0) {
		return err;
	}
	
	net_conf.guru_dev = &self->guru_dev;
	net_conf.msg = &self->lin_msg_net;
	
	for (idx = 0; idx < LINGURU_CHANNEL_COUNT; idx++) {
		net_conf.channel_idx = idx;
		linguru_net_create(&self->netdev[idx], &net_conf);
	}

	err = linguru_volt_create(&self->meas_volt_dev, &volt_conf);
	if (err != 0) {
		return err;
	}

	msg_conf.meas_volt_dev = self->meas_volt_dev;
	err = linguru_msg_net_init(&self->lin_msg_net, &msg_conf);
	if (err != 0) {
		return err;
	}

	return 0;
}

static void linguru_dev_deinit(struct linguru_device *self)
{
	int idx;
	
	for (idx = 0; idx < LINGURU_CHANNEL_COUNT; idx++) {
		linguru_net_destroy(&self->netdev[idx]);
	}

	linguru_volt_destroy(&self->meas_volt_dev);
}
