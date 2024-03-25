#include "mbus-device.h"

#include "canguru-net.h"
#include "linguru-net.h"

static void mbus_dev_deinit(struct mbus_device *self);
static int mbus_dev_init_can(struct mbus_device *self);

static int mbus_dev_init_lin(struct mbus_device *self);
static void mbus_dev_deinit_can(struct mbus_device *self);
static void mbus_dev_deinit_lin(struct mbus_device *self);

int mbus_dev_init(struct mbus_device *self, struct usb_interface *intf, struct class *guru_class)
{
	int err;

	self->guru_dev.dev_data = self;
	self->guru_dev.dev_deinit = (void (*)(void *))mbus_dev_deinit;

	err = guru_dev_init(&self->guru_dev, intf, guru_class);
	if (err != 0) {
		return err;
	}

	err = mbus_dev_init_can(self);
	if (err != 0) {
		return err;
	}
	err = mbus_dev_init_lin(self);
	if (err != 0) {
		return err;
	}
	return 0;
}

static int mbus_dev_init_can(struct mbus_device *self)
{
	struct canguru_channel_conf can_net_conf;
	const struct canguru_msg_net_conf can_msg_conf = {.guru_dev = &self->guru_dev,
													  .netdev = self->can_netdev,
													  .iface_count = CAN_MBUS_CHANNEL_COUNT};
	int idx;
	int err;

	for (idx = 0; idx < CAN_MBUS_CHANNEL_COUNT; idx++) {
		self->can_netdev[idx] = NULL;
	}

	can_net_conf.guru_dev = &self->guru_dev;
	can_net_conf.msg = &self->can_msg_net;

	err = canguru_msg_net_init(&self->can_msg_net, &can_msg_conf);
	if (err != 0) {
		return err;
	}

	for (idx = 0; idx < CAN_MBUS_CHANNEL_COUNT; idx++) {
		can_net_conf.channel_idx = idx;
		canguru_net_create(&self->can_netdev[idx], &can_net_conf);
	}

	return 0;
}

static int mbus_dev_init_lin(struct mbus_device *self)
{
	struct linguru_channel_conf lin_net_conf;
	const struct linguru_msg_net_conf lin_msg_conf = {.guru_dev = &self->guru_dev,
												.netdev = self->lin_netdev,
												.iface_count = LIN_MBUS_CHANNEL_COUNT,
												.meas_volt_dev = NULL};
	int idx;
	int err;

	for (idx = 0; idx < LIN_MBUS_CHANNEL_COUNT; idx++) {
		self->lin_netdev[idx] = NULL;
	}

	lin_net_conf.guru_dev = &self->guru_dev;
	lin_net_conf.msg = &self->lin_msg_net;

	for (idx = 0; idx < LIN_MBUS_CHANNEL_COUNT; idx++) {
		lin_net_conf.channel_idx = idx;
		linguru_net_create(&self->lin_netdev[idx], &lin_net_conf);
	}

	err = linguru_msg_net_init(&self->lin_msg_net, &lin_msg_conf);
	if (err != 0) {
		return err;
	}

	return 0;
}
static void mbus_dev_deinit(struct mbus_device *self)
{
	mbus_dev_deinit_can(self);
	mbus_dev_deinit_lin(self);
}

static void mbus_dev_deinit_can(struct mbus_device *self)
{
	int idx;

	for (idx = 0; idx < CAN_MBUS_CHANNEL_COUNT; idx++) {
		canguru_net_destroy(&self->can_netdev[idx]);
	}
}

static void mbus_dev_deinit_lin(struct mbus_device *self)
{
	int idx;

	for (idx = 0; idx < LIN_MBUS_CHANNEL_COUNT; idx++) {
		linguru_net_destroy(&self->lin_netdev[idx]);
	}
}
