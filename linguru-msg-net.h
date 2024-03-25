#ifndef LINGURU_MSG_NET_H
#define LINGURU_MSG_NET_H

#include "guru-msg.h"

struct guru_device;
struct iio_dev;

enum guru_msg_lin_id {
	LIN_MSG_CONFIG_SET = 0x0D00,
	LIN_MSG_CONFIG_GET,
	LIN_MSG_ENABLE_SET,
	LIN_MSG_ENABLE_GET,
	LIN_MSG_MASTER_ENABLE_SET,
	LIN_MSG_MASTER_ENABLE_GET,
	LIN_MSG_SEND,
	LIN_MSG_RECV,
	LIN_MSG_RESET,
	LIN_MSG_EVENT_ENABLE = 0x0D0E,
	LIN_MSG_EVENT,
	LIN_MSG_SLV_ANS_ADD,
	LIN_MSG_SLV_ANS_CLEAR,
	LIN_MSG_SLV_ANS_REMOVE,
	LIN_MSG_MASTER_ADD,
	LIN_MSG_MASTER_CLEAR,
	LIN_MSG_STATUS_GET,
	LIN_MSG_STATUS_CHANGED,
	LIN_MSG_VOLT_GET = 0x0D20
};

struct linguru_msg_net_conf {
	u8 iface_count;
	struct net_device **netdev;
	struct iio_dev *meas_volt_dev;
	struct guru_device *guru_dev;
};

struct linguru_msg_net
{
	u8 iface_count;
	struct net_device **netdev;
	struct iio_dev *meas_volt_dev;
	struct guru_device *guru_dev;

	/* LinGuru answer */
	struct guru_msg msg_reset;
	struct guru_msg msg_set_conf;
	struct guru_msg msg_set_enable;
	struct guru_msg msg_set_master_enable;
	struct guru_msg msg_send;
	struct guru_msg msg_recv;
	struct guru_msg msg_get_volt;
	
	struct list_head send_reply_list;
};

int linguru_msg_net_init(struct linguru_msg_net *self, const struct linguru_msg_net_conf *conf);
void linguru_wait_send_reply(struct linguru_msg_net *self, struct guru_send_reply *reply);

#endif // LINGURU_MSG_NET_H
