#ifndef MBUS_DEVICE_H
#define MBUS_DEVICE_H

#include "canguru-msg-net.h"
#include "linguru-msg-net.h"
#include "guru-device.h"

#define CAN_MBUS_CHANNEL_COUNT 8
#define LIN_MBUS_CHANNEL_COUNT 8

struct mbus_device
{
	/* must be the first member */
	struct guru_device guru_dev;
	
	struct net_device *can_netdev[CAN_MBUS_CHANNEL_COUNT];
	struct canguru_msg_net can_msg_net;

	struct net_device *lin_netdev[LIN_MBUS_CHANNEL_COUNT];
	struct linguru_msg_net lin_msg_net;
};

int mbus_dev_init(struct mbus_device *self, struct usb_interface *intf, struct class *guru_class);

#endif // MBUS_DEVICE_H
