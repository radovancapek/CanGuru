#ifndef LINGURU_NET_H
#define LINGURU_NET_H

#include <linux/can/dev.h>

#include "guru-msg.h"

struct guru_device;
struct linguru_msg_net;

struct linguru_channel_conf {
	struct guru_device *guru_dev;
	struct linguru_msg_net *msg;
	u8 channel_idx;
};

struct linguru_priv
{
	/* must be the first member */
	struct can_priv can;
	
	struct guru_device *guru_dev;
	struct linguru_msg_net *linguru_msg;
	u8 channel_idx;
	
	/* tx transfer */
	struct guru_send_reply reply;
};

int linguru_net_create(struct net_device **new_dev, const struct linguru_channel_conf *conf);
void linguru_net_destroy(struct net_device **netdev);
void linguru_net_recv(struct net_device *netdev, u8 *data);

#endif // LINGURU_NET_H
