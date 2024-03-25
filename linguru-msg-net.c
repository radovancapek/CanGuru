#include "linguru-msg-net.h"

#include "linguru-net.h"
#include "linguru-voltage.h"
#include "guru-device.h"

#define LINGURU_MSG_IFACE_IDX 0

struct __packed lin_status_msg {
	enum guru_status status : 8;
};

struct __packed lin_volt_msg {
	u32 voltage;
};

static void linguru_init_status_msg(struct linguru_msg_net *self,
									struct guru_msg *msg,
									u16 msgId);
static void linguru_status_callback(struct linguru_msg_net *self);
static void linguru_send_callback(struct linguru_msg_net *self);
static void linguru_recv_callback(struct linguru_msg_net *self);
static void linguru_volt_callback(struct linguru_msg_net *self);

/* LinGuru NET messages public methods implementation */

int linguru_msg_net_init(struct linguru_msg_net *self, const struct linguru_msg_net_conf *conf)
{
	self->iface_count = conf->iface_count;
	self->netdev = conf->netdev;
	self->guru_dev = conf->guru_dev;
	self->meas_volt_dev = conf->meas_volt_dev;

	INIT_LIST_HEAD(&self->send_reply_list);

	linguru_init_status_msg(self, &self->msg_reset, LIN_MSG_RESET);
	linguru_init_status_msg(self, &self->msg_set_conf, LIN_MSG_CONFIG_SET);
	linguru_init_status_msg(self, &self->msg_set_enable, LIN_MSG_ENABLE_SET);
	linguru_init_status_msg(self, &self->msg_set_master_enable, LIN_MSG_MASTER_ENABLE_SET);

	self->msg_send.size_restriction = GURU_SIZE_STATIC;
	self->msg_send.msgId = LIN_MSG_SEND;
	self->msg_send.user_data = self;
	self->msg_send.data_count = sizeof(struct lin_status_msg);
	self->msg_send.callback = (void (*)(void *))linguru_send_callback;
	guru_add_msg(conf->guru_dev, &self->msg_send);
	
	self->msg_recv.size_restriction = GURU_SIZE_VARIABLE;
	self->msg_recv.msgId = LIN_MSG_RECV;
	self->msg_recv.user_data = self;
	self->msg_recv.data_count = 0;
	self->msg_recv.callback = (void (*)(void *))linguru_recv_callback;
	guru_add_msg(conf->guru_dev, &self->msg_recv);

	self->msg_get_volt.size_restriction = GURU_SIZE_STATIC;
	self->msg_get_volt.msgId = LIN_MSG_VOLT_GET;
	self->msg_get_volt.user_data = self;
	self->msg_get_volt.data_count = sizeof(struct lin_volt_msg);
	self->msg_get_volt.callback = (void (*)(void *))linguru_volt_callback;
	guru_add_msg(conf->guru_dev, &self->msg_get_volt);
	return 0;
}

void linguru_wait_send_reply(struct linguru_msg_net *self, struct guru_send_reply *reply)
{
	list_add_tail(&reply->node, &self->send_reply_list);
}

/* LinGuru NET messages private methods implementation */

static void linguru_init_status_msg(struct linguru_msg_net *self, struct guru_msg *msg, u16 msgId)
{
	msg->size_restriction = GURU_SIZE_STATIC;
	msg->msgId = msgId;
	msg->user_data = self;
	msg->data_count = sizeof(struct lin_status_msg);
	msg->callback = (void (*)(void *))linguru_status_callback;
	
	guru_add_msg(self->guru_dev, msg);
}

static void linguru_status_callback(struct linguru_msg_net *self)
{
	struct lin_status_msg *msg = (struct lin_status_msg *)guru_get_msg_data(self->guru_dev);
	enum guru_msg_lin_id msgId = (enum guru_msg_lin_id)guru_get_msg_id(self->guru_dev);
	
	if (msg->status != GURU_STATUS_DONE) {
		dev_warn(self->guru_dev->dev, "Command ID 0x%x status %u", msgId, msg->status);
	}
}

static void linguru_send_callback(struct linguru_msg_net *self)
{
	struct lin_status_msg *msg = (struct lin_status_msg *)guru_get_msg_data(self->guru_dev);
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

static void linguru_recv_callback(struct linguru_msg_net *self)
{
	u8 *data = guru_get_msg_data(self->guru_dev);
	u8 iface = data[LINGURU_MSG_IFACE_IDX];
	
	if (iface >= self->iface_count) {
		dev_warn(self->guru_dev->dev,
				 "LIN iface value %u is out of range (count of iface %u).",
				 iface,
				 self->iface_count);
		return;
	}
	
	linguru_net_recv(self->netdev[iface], data);
}

static void linguru_volt_callback(struct linguru_msg_net *self)
{
	struct lin_volt_msg *data = (struct lin_volt_msg *)guru_get_msg_data(self->guru_dev);

	if (self->meas_volt_dev == NULL) {
		dev_warn(self->guru_dev->dev, "LinGuru voltage dev is invalid.");
		return;
	}
	linguru_volt_reply(self->meas_volt_dev, data->voltage);
}
