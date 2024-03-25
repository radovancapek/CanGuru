#include "canguru-msg-net.h"

#include "canguru-net.h"
#include "guru-device.h"

#define CANGURU_MSG_IFACE_IDX 0
#define CANGURU_MSG_DATA_IDX 1

struct __packed can_status_msg {
	enum guru_status status : 8;
};

static void canguru_init_status_msg(struct canguru_msg_net *self,
									struct guru_msg *msg,
									u16 msgId);
static void canguru_status_callback(struct canguru_msg_net *self);
static void canguru_send_callback(struct canguru_msg_net *self);
static void canguru_recv_callback(struct canguru_msg_net *self);

/* CanGuru NET messages public methods implementation */

int canguru_msg_net_init(struct canguru_msg_net *self, const struct canguru_msg_net_conf *conf)
{
	self->iface_count = conf->iface_count;
	self->netdev = conf->netdev;
	self->guru_dev = conf->guru_dev;
	
	INIT_LIST_HEAD(&self->send_reply_list);

	canguru_init_status_msg(self, &self->msg_reset, CAN_MSG_RESET);
	canguru_init_status_msg(self, &self->msg_set_conf, CAN_MSG_CONFIG_SET);
	canguru_init_status_msg(self, &self->msg_set_timing, CAN_MSG_TIMING_SET);
	canguru_init_status_msg(self, &self->msg_set_enable, CAN_MSG_ENABLE_SET);
	canguru_init_status_msg(self, &self->msg_set_ter, CAN_MSG_TER_ENABLE);
	canguru_init_status_msg(self, &self->msg_set_global_filter, CAN_MSG_GLOBAL_FILTER_SET);

	self->msg_send.size_restriction = GURU_SIZE_STATIC;
	self->msg_send.msgId = CAN_MSG_SEND;
	self->msg_send.user_data = self;
	self->msg_send.data_count = sizeof(struct can_status_msg);
	self->msg_send.callback = (void (*)(void *))canguru_send_callback;
	guru_add_msg(conf->guru_dev, &self->msg_send);
	
	self->msg_recv.size_restriction = GURU_SIZE_VARIABLE;
	self->msg_recv.msgId = CAN_MSG_RECV;
	self->msg_recv.user_data = self;
	self->msg_recv.data_count = 0;
	self->msg_recv.callback = (void (*)(void *))canguru_recv_callback;
	guru_add_msg(conf->guru_dev, &self->msg_recv);

	return 0;
}

void canguru_wait_send_reply(struct canguru_msg_net *self, struct guru_send_reply *reply)
{
	list_add_tail(&reply->node, &self->send_reply_list);
}

/* CanGuru NET messages private methods implementation */

static void canguru_init_status_msg(struct canguru_msg_net *self, struct guru_msg *msg, u16 msgId)
{
	msg->size_restriction = GURU_SIZE_STATIC;
	msg->msgId = msgId;
	msg->user_data = self;
	msg->data_count = sizeof(struct can_status_msg);
	msg->callback = (void (*)(void *))canguru_status_callback;

	guru_add_msg(self->guru_dev, msg);
}

static void canguru_status_callback(struct canguru_msg_net *self)
{
	struct can_status_msg *msg = (struct can_status_msg *)guru_get_msg_data(self->guru_dev);
	enum guru_msg_can_id msgId = (enum guru_msg_can_id)guru_get_msg_id(self->guru_dev);
	
	if (msg->status != GURU_STATUS_DONE) {
		dev_warn(self->guru_dev->dev, "Command ID 0x%x status %u", msgId, msg->status);
	}
}

static void canguru_send_callback(struct canguru_msg_net *self)
{
	struct can_status_msg *msg = (struct can_status_msg *)guru_get_msg_data(self->guru_dev);
	struct guru_send_reply *reply;
	
	if (list_empty(&self->send_reply_list) == true) {
		return;
	}
	
	reply = list_first_entry(&self->send_reply_list,
							 struct guru_send_reply,
							 node);
	
	if (reply->callback != NULL) {
		reply->callback(reply->user_data, msg->status);
	}
	list_del(&reply->node);
}

static void canguru_recv_callback(struct canguru_msg_net *self)
{
	u8 *data = guru_get_msg_data(self->guru_dev);
	u8 iface = data[CANGURU_MSG_IFACE_IDX];

	if (iface >= self->iface_count) {
		dev_warn(self->guru_dev->dev,
				 "CAN iface value %u is out of range (count of iface %u).",
				 iface,
				 self->iface_count);
		return;
	}

	canguru_net_recv(self->netdev[iface], data);
}
