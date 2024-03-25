#ifndef CANGURU_MSG_NET_H
#define CANGURU_MSG_NET_H

#include "guru-msg.h"

struct guru_device;

enum guru_msg_can_id
{
	CAN_MSG_CONFIG_SET = 0x0C00,
	CAN_MSG_CONFIG_GET,
	CAN_MSG_GLOBAL_FILTER_SET,
	CAN_MSG_GLOBAL_FILTER_GET,
	CAN_MSG_FILTER_SET,
	CAN_MSG_FILTER_GET,
	CAN_MSG_ENABLE_SET,
	CAN_MSG_ENABLE_GET,
	CAN_MSG_RECV_ENABLE_SET,
	CAN_MSG_RECV_ENABLE_GET,
	CAN_MSG_SEND,
	CAN_MSG_RECV,
	CAN_MSG_RELOAD,
	CAN_MSG_RESET,
	CAN_MSG_EVENT,
	CAN_MSG_BRIDGE_MODE_SELECT,
	CAN_MSG_BRIDGE_LIST_CLEAR,
	CAN_MSG_BRIDGE_LIST_ADD,
	CAN_MSG_STATUS_GET,
	CAN_MSG_STATUS_CHANGED,
	CAN_MSG_EVENT_ENABLE = 0x0C15,
	CAN_MSG_TER_ENABLE,
	CAN_MSG_TIMING_SET,
	CAN_MSG_TIMING_GET
};

struct canguru_msg_net_conf {
	u8 iface_count;
	struct net_device **netdev;
	struct guru_device *guru_dev;
};

struct canguru_msg_net {
	u8 iface_count;
	struct net_device **netdev;
	struct guru_device *guru_dev;

	/* CanGuru answer */
	struct guru_msg msg_reset;
	struct guru_msg msg_set_conf;
	struct guru_msg msg_set_timing;
	struct guru_msg msg_set_global_filter;
	struct guru_msg msg_set_enable;
	struct guru_msg msg_set_ter;
	struct guru_msg msg_send;
	struct guru_msg msg_recv;
	
	struct list_head send_reply_list;
};

int canguru_msg_net_init(struct canguru_msg_net *self, const struct canguru_msg_net_conf *conf);
void canguru_wait_send_reply(struct canguru_msg_net *self, struct guru_send_reply *reply);

#endif // CANGURU_MSG_NET_H
