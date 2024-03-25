#ifndef CANGURU_NET_H
#define CANGURU_NET_H

#include <linux/can/dev.h>

#include "guru-msg.h"

struct guru_device;
struct canguru_msg_net;

struct canguru_channel_conf {
	struct guru_device *guru_dev;
	struct canguru_msg_net *msg;
	u8 channel_idx;
};

struct canguru_priv {
	/* must be the first member */
	struct can_priv can;

	struct guru_device *guru_dev;
	struct canguru_msg_net *canguru_msg;
	u8 channel_idx;
	
	/* tx transfer */
	struct guru_send_reply reply;
};

int canguru_net_create(struct net_device **new_dev, const struct canguru_channel_conf *conf);
void canguru_net_destroy(struct net_device **netdev);
void canguru_net_recv(struct net_device *netdev, u8 *data);

#endif // CANGURU_NET_H
