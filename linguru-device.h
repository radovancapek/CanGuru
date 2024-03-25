#ifndef LINGURU_DEVICE_H
#define LINGURU_DEVICE_H

#include "linguru-msg-net.h"
#include "guru-device.h"

#define LINGURU_CHANNEL_COUNT 10

struct linguru_device
{
	/* must be the first member */
	struct guru_device guru_dev;
	
	struct net_device *netdev[LINGURU_CHANNEL_COUNT];
	struct iio_dev *meas_volt_dev;
	struct linguru_msg_net lin_msg_net;
};

int linguru_dev_init(struct linguru_device *self,
					 struct usb_interface *intf,
					 struct class *guru_class);

#endif // LINGURU_DEVICE_H
