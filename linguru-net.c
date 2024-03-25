#include "linguru-net.h"

#include "guru-device.h"
#include "linguru-msg-net.h"

#define LIN_MAX_DLEN 8
#define LINGURU_100US_TO_1NS 100000
#define LINGURU_MASTER_DISABLED CAN_TERMINATION_DISABLED
#define LINGURU_MASTER_ENABLED 1000 /* master pull-up 1k */
#define LIN_IFACE_NAME "lin"
#define LIN_ID_MASK 0x3fU
#define LIN_ENHANCED_SUM_FLAG 0x40

static const u16 linguru_termination[] = {LINGURU_MASTER_DISABLED, LINGURU_MASTER_ENABLED};
static const u32 linguru_arb_bitrate[] = {9600, 19200, 20000};


struct __packed lin_recv_header {
	u8 iface;
	u32 timestamp;
};

struct __packed lin_msg_header {
	u32 id : 6;
	bool remote_data : 1;
	bool enhanced_sum : 1;
	u8 data_len;
};

struct __packed lin_msg_response {
	u8 data[LIN_MAX_DLEN];
};

struct __packed lin_msg {
	struct lin_msg_header header;
	struct lin_msg_response response;
};

struct __packed lin_msg_recv {
	struct lin_recv_header recv_header;
	struct lin_msg msg;
};

struct __packed lin_master_header {
	u8 iface;
	u32 delay;
};

struct __packed lin_msg_master {
	struct lin_master_header send_header;
	struct lin_msg msg;
};

struct __packed lin_enable_msg {
	u8 iface;
	bool enable;
};

enum lin_mode
{
	LIN_MODE_MONITORING = 0x0,
	LIN_MODE_SLAVE,
	LIN_MODE_MASTER_USER,
	LIN_MODE_MASTER_AUTO
};

struct __packed lin_conf_msg {
	u8 iface;
	u32 baudrate;
	enum lin_mode mode : 8;
};

static void linguru_init_can_priv(struct linguru_priv *priv);
static int linguru_ops_open(struct net_device *netdev);
static int linguru_ops_close(struct net_device *netdev);
static netdev_tx_t linguru_start_xmit(struct sk_buff *skb, struct net_device *netdev);

static int linguru_set_termination(struct net_device *netdev, u16 term);
static int linguru_set_mode(struct net_device *netdev, enum can_mode mode);
static int linguru_set_bittiming(struct net_device *netdev);

static int linguru_set_lin_conf(struct linguru_priv *priv, const struct lin_conf_msg *value);
static int linguru_lin_reset(struct linguru_priv *priv);
static int linguru_set_lin_enable(struct linguru_priv *priv, bool enable);
static int linguru_set_lin_master(struct linguru_priv *priv, bool enable);
static int linguru_lin_send(struct linguru_priv *priv, struct can_frame *frame);
static void linguru_send_callback(struct linguru_priv *priv, enum guru_status status);

static inline struct linguru_priv *linguru_get_priv(struct net_device *netdev)
{
	return (struct linguru_priv *)netdev_priv(netdev);
}

static const struct net_device_ops linguru_netdev_ops = {.ndo_open = linguru_ops_open,
														 .ndo_stop = linguru_ops_close,
														 .ndo_start_xmit = linguru_start_xmit};

/* LinGuru net public methods implementation */

int linguru_net_create(struct net_device **new_dev, const struct linguru_channel_conf *conf)
{
	struct net_device *netdev;
	struct linguru_priv *priv;
	int err;

	netdev = alloc_candev(sizeof(struct linguru_priv), GURU_MAX_URBS);
	if (netdev == NULL) {
		dev_err(conf->guru_dev->dev, "Couldn't alloc lindev\n");
		return -ENOMEM;
	}

	SET_NETDEV_DEV(netdev, conf->guru_dev->dev);
	priv = linguru_get_priv(netdev);
	priv->guru_dev = conf->guru_dev;
	priv->linguru_msg = conf->msg;
	priv->channel_idx = conf->channel_idx;
	priv->reply.user_data = priv;
	priv->reply.callback = (void (*)(void *, enum guru_status))linguru_send_callback;
	linguru_init_can_priv(priv);
	err = linguru_lin_reset(priv);
	if (err != 0) {
		return err;
	}

	memcpy(netdev->name, LIN_IFACE_NAME, strlen(LIN_IFACE_NAME));
	/* IFF_ECHO must be set if can_put_echo_skb and can_get_echo_skb is used */
	netdev->flags |= IFF_ECHO;
	netdev->netdev_ops = &linguru_netdev_ops;
	err = register_candev(netdev);
	if (err != 0) {
		return err;
	}
	*new_dev = netdev;
	return 0;
}

void linguru_net_destroy(struct net_device **netdev)
{
	unregister_candev(*netdev);
	free_candev(*netdev);
	*netdev = NULL;
}

void linguru_net_recv(struct net_device *netdev, u8 *data)
{
	struct linguru_priv *priv = linguru_get_priv(netdev);
	struct lin_msg_recv *msg = (struct lin_msg_recv *)data;
	struct skb_shared_hwtstamps *hwts;
	struct can_frame *frame_cc;
	struct sk_buff *skb;
	u64 time_stamp;
	u8 dataLen;

	dataLen = msg->msg.header.data_len;
	skb = alloc_can_skb(netdev, &frame_cc);
	frame_cc->can_id = msg->msg.header.id;
	can_frame_set_cc_len(frame_cc, msg->msg.header.data_len, priv->can.ctrlmode);

	if (msg->msg.header.remote_data == true) {
		frame_cc->can_id |= CAN_RTR_FLAG;
	}
	if (msg->msg.header.enhanced_sum == true) {
		frame_cc->can_id |= LIN_ENHANCED_SUM_FLAG;
	}
	memcpy(frame_cc->data, msg->msg.response.data, dataLen);

	netdev->stats.rx_packets++;
	netdev->stats.rx_bytes += dataLen;
	hwts = skb_hwtstamps(skb);
	time_stamp = msg->recv_header.timestamp;
	time_stamp *= LINGURU_100US_TO_1NS;
	hwts->hwtstamp = ns_to_ktime(time_stamp);
	netif_rx(skb);
}

/* LinGuru net private methods implementation */

static void linguru_init_can_priv(struct linguru_priv *priv)
{
	priv->can.state = CAN_STATE_STOPPED;
	priv->can.termination_const = linguru_termination;
	priv->can.termination_const_cnt = ARRAY_SIZE(linguru_termination);
	priv->can.bitrate_const = linguru_arb_bitrate;
	priv->can.bitrate_const_cnt = ARRAY_SIZE(linguru_arb_bitrate);
	priv->can.ctrlmode_supported = CAN_CTRLMODE_LISTENONLY
								   | CAN_CTRLMODE_CC_LEN8_DLC;

	priv->can.do_set_termination = linguru_set_termination;
	priv->can.do_set_mode = linguru_set_mode;
	priv->can.do_set_bittiming = linguru_set_bittiming;
}

static int linguru_ops_open(struct net_device *netdev)
{
	struct linguru_priv *priv = linguru_get_priv(netdev);
	const struct lin_conf_msg conf = {.iface = priv->channel_idx,
									  .baudrate = priv->can.bittiming.bitrate,
									  .mode = (priv->can.ctrlmode & CAN_CTRLMODE_LISTENONLY) != 0
												  ? LIN_MODE_MONITORING
												  : LIN_MODE_MASTER_USER};
	int err;
	
	err = open_candev(netdev);
	if (err != 0) {
		return err;
	}
	
	err = linguru_set_lin_conf(priv, &conf);
	if (err != 0) {
		dev_warn(priv->guru_dev->dev, "Unable to set config LIN bus");
		return err;
	}
	
	dev_info(priv->guru_dev->dev,
			 "Set LIN config(iface=%u, baudrate=%u, mode=%u)",
			 conf.iface,
			 conf.baudrate,
			 conf.mode);

	priv->can.state = CAN_STATE_ERROR_ACTIVE;
	netif_start_queue(netdev);
	linguru_set_lin_enable(priv, true);
	return 0;
}

static int linguru_ops_close(struct net_device *netdev)
{
	struct linguru_priv *priv = linguru_get_priv(netdev);

	linguru_set_lin_enable(priv, false);
	priv->can.state = CAN_STATE_STOPPED;
	netif_stop_queue(netdev);
	close_candev(netdev);
	return 0;
}

static netdev_tx_t linguru_start_xmit(struct sk_buff *skb, struct net_device *netdev)
{
	struct linguru_priv *priv = linguru_get_priv(netdev);
	u32 frame_len;
	int err;

	if (can_dropped_invalid_skb(netdev, skb)) {
		return NETDEV_TX_OK;
	}

	netif_stop_queue(netdev);
	err = linguru_lin_send(priv, (struct can_frame *)skb->data);
	if (err != 0) {
		goto send_err;
	}

	frame_len = can_skb_get_frame_len(skb);
	err = can_put_echo_skb(skb, netdev, 0, frame_len);
	if (err != 0) {
		goto fail;
	}

	linguru_wait_send_reply(priv->linguru_msg, &priv->reply);

	netdev_sent_queue(netdev, frame_len);
	return NETDEV_TX_OK;
send_err:
	netif_wake_queue(netdev);
fail:
	dev_warn(priv->guru_dev->dev, "Unable to send CAN frame.");
	return NETDEV_TX_OK;
}

static int linguru_set_termination(struct net_device *netdev, u16 term)
{
	struct linguru_priv *priv = linguru_get_priv(netdev);
	bool terEnable = term != 0;

	return linguru_set_lin_master(priv, terEnable);
}

static int linguru_set_mode(__maybe_unused struct net_device *netdev,
							__maybe_unused enum can_mode mode)
{
	return 0;
}

static int linguru_set_bittiming(__maybe_unused struct net_device *netdev)
{
	return 0;
}

static int linguru_set_lin_conf(struct linguru_priv *priv, const struct lin_conf_msg *value)
{
	int err;

	err = guru_send_data(priv->guru_dev,
						 LIN_MSG_CONFIG_SET,
						 (u8 *)value,
						 sizeof(struct lin_conf_msg));
	return err;
}

static int linguru_lin_reset(struct linguru_priv *priv)
{
	return guru_send_data(priv->guru_dev, LIN_MSG_RESET, &priv->channel_idx, sizeof(u8));
}

static int linguru_set_lin_enable(struct linguru_priv *priv, bool enable)
{
	const struct lin_enable_msg msg = {.iface = priv->channel_idx, .enable = enable};
	
	dev_info(priv->guru_dev->dev,
			 "Set LIN enable(iface=%u, enable=%u)",
			 msg.iface,
			 msg.enable);
	return guru_send_data(priv->guru_dev,
						  LIN_MSG_ENABLE_SET,
						  (u8 *)&msg,
						  sizeof(struct lin_enable_msg));
}

static int linguru_set_lin_master(struct linguru_priv *priv, bool enable)
{
	const struct lin_enable_msg msg = {.iface = priv->channel_idx, .enable = enable};

	return guru_send_data(priv->guru_dev,
						  LIN_MSG_MASTER_ENABLE_SET,
						  (u8 *)&msg,
						  sizeof(struct lin_enable_msg));
}

static int linguru_lin_send(struct linguru_priv *priv, struct can_frame *frame)
{
	struct lin_msg_master msg = {0};

	msg.send_header.iface = priv->channel_idx;
	msg.send_header.delay = 0;

	msg.msg.header.data_len = can_get_cc_dlc(frame, priv->can.ctrlmode);

	msg.msg.header.remote_data = (frame->can_id & CAN_RTR_FLAG) != 0;
	msg.msg.header.enhanced_sum = (frame->can_id & LIN_ENHANCED_SUM_FLAG) != 0;
	msg.msg.header.id = frame->can_id & LIN_ID_MASK;
	
	memcpy(msg.msg.response.data, frame->data, frame->len);
	return guru_send_data(priv->guru_dev,
						  LIN_MSG_SEND,
						  (const uint8_t *)&msg,
						  sizeof(struct lin_master_header) + sizeof(struct lin_msg_header)
							  + frame->len);
}

static void linguru_send_callback(struct linguru_priv *priv, enum guru_status status)
{
	struct net_device *netdev = priv->can.dev;
	unsigned int byte_count;
	unsigned int frame_len;
	
	byte_count = can_get_echo_skb(netdev, 0, &frame_len);
	switch (status) {
	case GURU_STATUS_DONE:
		netdev->stats.tx_packets++;
		netdev->stats.tx_bytes += byte_count;
		break;
	case GURU_STATUS_ERROR:
		netdev->stats.tx_errors++;
		break;
	default:
		netdev->stats.tx_dropped++;
		break;
	}
	
	netdev_completed_queue(netdev, 1, frame_len);
	netif_wake_queue(netdev);
}
