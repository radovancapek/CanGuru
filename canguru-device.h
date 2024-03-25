#ifndef CANGURU_DEVICE_H
#define CANGURU_DEVICE_H

#include "canguru-msg-net.h"
#include "guru-boot.h"
#include "guru-device.h"

#define CANGURU_CHANNEL_COUNT 2

struct canguru_device
{
	/* must be the first member */
	struct guru_device guru_dev;
	
	struct net_device *netdev[CANGURU_CHANNEL_COUNT];
	struct canguru_msg_net can_msg_net;
	struct guru_boot boot;
};

int canguru_dev_init(struct canguru_device *self,
					 struct usb_interface *intf,
					 struct class *guru_class);

#endif // CANGURU_DEVICE_H
