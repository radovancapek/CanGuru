#include "canguru-net.h"

#include "guru-device.h"
#include "canguru-msg-net.h"

#define CANGURU_CAN_CLK 80000000
#define CANGURU_TERMINATION_DISABLED CAN_TERMINATION_DISABLED
#define CANGURU_TERMINATION_ENABLED 120
#define CANGURU_100US_TO_1NS 100000
#define CAN_STD_ID_MASK 0x7ffU
#define CAN_EXT_ID_MASK 0x1fffffffU

static const u16 canguru_termination[] = {CANGURU_TERMINATION_DISABLED, CANGURU_TERMINATION_ENABLED};

static const u32 canguru_arb_bitrate[] = {125000, 250000, 500000, 1000000};
static const u32 canguru_data_bitrate[]
	= {125000, 250000, 500000, 1000000, 2000000, 4000000, 5000000};

static const struct can_bittiming_const canguru_bittiming_arb = {
	.name = "canguru_arb",
	.tseg1_min = 2,
	.tseg1_max = 256,
	.tseg2_min = 2,
	.tseg2_max = 128,
	.sjw_max = 128,
	.brp_min = 1,
	.brp_max = 512,
	.brp_inc = 1,
};

static const struct can_bittiming_const canguru_bittiming_data = {
	.name = "canguru_data",
	.tseg1_min = 1,
	.tseg1_max = 32,
	.tseg2_min = 1,
	.tseg2_max = 16,
	.sjw_max = 16,
	.brp_min = 1,
	.brp_max = 32,
	.brp_inc = 1,
};

enum can_conf_mode
{
	CAN_NORMAL = 0,
	CAN_BUS_MONITORING,
	CAN_RESTRICTED_OPERATION
};

struct __packed can_msg_header {
	u32 can_id : 29;
	u32 can_dlc : 4;
	bool extended_id : 1;
	bool remote_frame : 1;
	bool esi : 1;
	bool canfd : 1;
	bool bitrate_switch : 1;
	u32 reserved : 2;
};

struct __packed can_recv_header {
	u8 iface;
	u32 time_stamp;
	struct can_msg_header can_header;
};

struct __packed can_recv_msg {
	struct can_recv_header recv_header;
	u8 data[CANFD_MAX_DLEN];
};

struct __packed can_send_header {
	u8 iface;
	struct can_msg_header can_header;
};

struct __packed can_send_msg {
	struct can_send_header send_header;
	u8 data[CANFD_MAX_DLEN];
};

struct __packed can_enable_msg {
	u8 iface;
	bool enable;
};

struct __packed can_conf_msg {
	u8 iface;
	u32 baudRateA; /* Baud rate of Arbitration phase in bps. */
	u32 baudRateD; /* Baud rate of Data phase in bps. */
	u8 canFdEnable; /* Enable or Disable CANFD. */
	enum can_conf_mode canMode : 8;
};

struct can_timing {
	uint32_t samplePoint;
	uint32_t idealTqNum;
	uint32_t syncJumpWidth;
};

struct __packed can_timing_msg {
	u8 iface;
	struct can_timing timingNominal;
	struct can_timing timingData;
};

struct __packed can_global_filter_msg {
	u8 iface;
	bool stdDefaultAllow; /* All standard frames are by default accepted */
	bool extDefaultAllow; /* All extended frames are by default accepted */
	bool stdRemoteReject; /* Reject all standard remote frames */
	bool extRemoteReject; /* Reject all extended remote frames */
};

static void canguru_init_can_priv(struct canguru_priv *priv);

static int canguru_ops_open(struct net_device *netdev);
static int canguru_ops_close(struct net_device *netdev);
static netdev_tx_t canguru_start_xmit(struct sk_buff *skb, struct net_device *netdev);

static int canguru_set_termination(struct net_device *netdev, u16 term);
static int canguru_set_mode(struct net_device *netdev, enum can_mode mode);
static int canguru_set_bittiming(struct net_device *netdev);
static int canguru_set_can_conf(struct canguru_priv *priv, const struct can_conf_msg *value);
static int canguru_set_can_timing(struct canguru_priv *priv, const struct can_timing_msg *value);
static int canguru_set_can_enable(struct canguru_priv *priv, bool enable);
static int canguru_set_can_ter(struct canguru_priv *priv, bool enable);
static int canguru_can_reset(struct canguru_priv *priv);
static int canguru_set_global_filter(struct canguru_priv *priv);
static int canguru_can_fd_send(struct canguru_priv *priv, struct canfd_frame *frame);
static int canguru_can_classic_send(struct canguru_priv *priv, struct can_frame *frame);
static void canguru_send_callback(struct canguru_priv *priv, enum guru_status status);


static inline struct canguru_priv *canguru_get_priv(struct net_device *netdev)
{
	return (struct canguru_priv *)netdev_priv(netdev);
}

static const struct net_device_ops canguru_netdev_ops = {.ndo_open = canguru_ops_open,
														 .ndo_stop = canguru_ops_close,
														 .ndo_start_xmit = canguru_start_xmit};

/* CanGuru net public methods implementation */

int canguru_net_create(struct net_device **new_dev, const struct canguru_channel_conf *conf)
{
	struct net_device *netdev;
	struct canguru_priv *priv;
	int err;

	netdev = alloc_candev(sizeof(struct canguru_priv), GURU_MAX_URBS);
	if (netdev == NULL) {
		dev_err(conf->guru_dev->dev, "Couldn't alloc candev\n");
		return -ENOMEM;
	}

	SET_NETDEV_DEV(netdev, conf->guru_dev->dev);
	priv = canguru_get_priv(netdev);
	priv->guru_dev = conf->guru_dev;
	priv->canguru_msg = conf->msg;
	priv->channel_idx = conf->channel_idx;
	priv->reply.user_data = priv;
	priv->reply.callback = (void (*)(void *, enum guru_status))canguru_send_callback;
	canguru_init_can_priv(priv);
	err = canguru_can_reset(priv);
	if (err != 0) {
		return err;
	}

	err = canguru_set_global_filter(priv);
	if (err != 0) {
		return err;
	}

	/* IFF_ECHO must be set if can_put_echo_skb and can_get_echo_skb is used */
	netdev->flags |= IFF_ECHO;
	netdev->netdev_ops = &canguru_netdev_ops;
	err = register_candev(netdev);
	if (err != 0) {
		return err;
	}
	*new_dev = netdev;
	return 0;
}

void canguru_net_destroy(struct net_device **netdev)
{
	unregister_candev(*netdev);
	free_candev(*netdev);
	*netdev = NULL;
}

void canguru_net_recv(struct net_device *netdev, u8 *data)
{
	struct canguru_priv *priv = canguru_get_priv(netdev);
	struct can_recv_msg *msg = (struct can_recv_msg *)data;
	struct canfd_frame *frame_fd;
	struct can_frame *frame_cc;
	struct sk_buff *skb;
	u8 dataLen;
	struct skb_shared_hwtstamps *hwts;
	u64 time_stamp;

	if (msg->recv_header.can_header.canfd == true) {
		dataLen = can_fd_dlc2len(msg->recv_header.can_header.can_dlc);
		skb = alloc_canfd_skb(netdev, &frame_fd);
		frame_fd->can_id = msg->recv_header.can_header.can_id;
		frame_fd->len = dataLen;

		if (msg->recv_header.can_header.esi == true) {
			frame_fd->flags |= CANFD_ESI;
		}
		if (msg->recv_header.can_header.bitrate_switch == true) {
			frame_fd->flags |= CANFD_BRS;
		}
		if (msg->recv_header.can_header.remote_frame == true) {
			frame_fd->can_id |= CAN_RTR_FLAG;
		}
		if (msg->recv_header.can_header.extended_id == true) {
			frame_fd->can_id |= CAN_EFF_FLAG;
		}
		memcpy(frame_fd->data, msg->data, dataLen);

	} else {
		dataLen = can_cc_dlc2len(msg->recv_header.can_header.can_dlc);
		skb = alloc_can_skb(netdev, &frame_cc);
		frame_cc->can_id = msg->recv_header.can_header.can_id;
		can_frame_set_cc_len(frame_cc, msg->recv_header.can_header.can_dlc, priv->can.ctrlmode);

		if (msg->recv_header.can_header.remote_frame == true) {
			frame_cc->can_id |= CAN_RTR_FLAG;
		}
		if (msg->recv_header.can_header.extended_id == true) {
			frame_cc->can_id |= CAN_EFF_FLAG;
		}
		memcpy(frame_cc->data, msg->data, dataLen);
	}

	netdev->stats.rx_packets++;
	netdev->stats.rx_bytes += dataLen;
	hwts = skb_hwtstamps(skb);
	time_stamp = msg->recv_header.time_stamp;
	time_stamp *= CANGURU_100US_TO_1NS;
	hwts->hwtstamp = ns_to_ktime(time_stamp);
	netif_rx(skb);
}

/* CanGuru net private methods implementation */

static void canguru_init_can_priv(struct canguru_priv *priv)
{
	priv->can.state = CAN_STATE_STOPPED;
	priv->can.clock.freq = CANGURU_CAN_CLK;
	priv->can.termination_const = canguru_termination;
	priv->can.termination_const_cnt = ARRAY_SIZE(canguru_termination);
	priv->can.bittiming_const = &canguru_bittiming_arb;
	priv->can.data_bittiming_const = &canguru_bittiming_data;
	priv->can.bitrate_const = canguru_arb_bitrate;
	priv->can.bitrate_const_cnt = ARRAY_SIZE(canguru_arb_bitrate);
	priv->can.data_bitrate_const = canguru_data_bitrate;
	priv->can.data_bitrate_const_cnt = ARRAY_SIZE(canguru_data_bitrate);
	priv->can.ctrlmode_supported = CAN_CTRLMODE_LISTENONLY | CAN_CTRLMODE_CC_LEN8_DLC
								   | CAN_CTRLMODE_FD;

	priv->can.do_set_termination = canguru_set_termination;
	priv->can.do_set_mode = canguru_set_mode;
	priv->can.do_set_bittiming = canguru_set_bittiming;
	priv->can.do_set_data_bittiming = canguru_set_bittiming;
}

/**
 * canguru_ops_open() - Enable the network device.
 * @netdev: CAN network device.
 *
 * Called when the network transitions to the up state. Allocate the
 * URB resources if needed and open the channel.
 */
static int canguru_ops_open(struct net_device *netdev)
{
	struct canguru_priv *priv = canguru_get_priv(netdev);
	int err;

	err = open_candev(netdev);
	if (err != 0) {
		return err;
	}

	priv->can.state = CAN_STATE_ERROR_ACTIVE;
	netif_start_queue(netdev);
	canguru_set_can_enable(priv, true);
	
	return 0;
}

/**
 * canguru_ops_close() - Disable the network device.
 * @netdev: CAN network device.
 *
 * Called when the network transitions to the down state. If all the
 * channels of the device are closed, free the URB resources which are
 * not needed anymore.
 */
static int canguru_ops_close(struct net_device *netdev)
{
	struct canguru_priv *priv = canguru_get_priv(netdev);
	
	canguru_set_can_enable(priv, false);
	priv->can.state = CAN_STATE_STOPPED;
	netif_stop_queue(netdev);
	close_candev(netdev);
	return 0;
}

/**
 * canguru_start_xmit() - Transmit an skb.
 * @skb: socket buffer of a CAN message.
 * @netdev: CAN network device.
 *
 * Called when a packet needs to be transmitted.
 */
static netdev_tx_t canguru_start_xmit(struct sk_buff *skb, struct net_device *netdev)
{
	struct canguru_priv *priv = canguru_get_priv(netdev);
	u32 frame_len;
	int err;

	if (can_dropped_invalid_skb(netdev, skb)) {
		return NETDEV_TX_OK;
	}

	netif_stop_queue(netdev);

	if (can_is_canfd_skb(skb) == true) {
		err = canguru_can_fd_send(priv, (struct canfd_frame *)skb->data);
	} else {
		err = canguru_can_classic_send(priv, (struct can_frame *)skb->data);
	}
	
	if (err != 0) {
		goto send_err;
	}
	
	frame_len = can_skb_get_frame_len(skb);
	err = can_put_echo_skb(skb, netdev,
					 0,
					 frame_len);
	if (err != 0) {
		goto fail;
	}
	
	canguru_wait_send_reply(priv->canguru_msg, &priv->reply);
	
	netdev_sent_queue(netdev, frame_len);
	return NETDEV_TX_OK;
send_err:
	netif_wake_queue(netdev);
fail:
	dev_warn(priv->guru_dev->dev, "Unable to send CAN frame.");
	return NETDEV_TX_OK;
}

static int canguru_set_termination(struct net_device *netdev, u16 term)
{
	struct canguru_priv *priv = canguru_get_priv(netdev);
	bool terEnable = term != 0;
	
	return canguru_set_can_ter(priv, terEnable);
}

static int canguru_set_mode(__maybe_unused struct net_device *netdev,
							__maybe_unused enum can_mode mode)
{
	return 0;
}

static int canguru_set_bittiming(struct net_device *netdev)
{
	struct canguru_priv *priv = canguru_get_priv(netdev);
	const u32 prescaler = priv->can.bittiming.brp + 1;
	const u32 prescalerData = priv->can.data_bittiming.brp + 1;
	const struct can_conf_msg conf = {.iface = priv->channel_idx,
									  .baudRateA = priv->can.bittiming.bitrate,
									  .baudRateD = priv->can.data_bittiming.bitrate,
									  .canFdEnable = (priv->can.ctrlmode & CAN_CTRLMODE_FD) != 0,
									  .canMode = (priv->can.ctrlmode & CAN_CTRLMODE_LISTENONLY) != 0
													 ? CAN_BUS_MONITORING
													 : CAN_NORMAL};
	struct can_timing_msg timingConf
		= {.iface = priv->channel_idx,
		   .timingNominal = {.samplePoint = priv->can.bittiming.sample_point,
							 .idealTqNum = 0,
							 .syncJumpWidth = priv->can.bittiming.sjw},
		   .timingData = {.samplePoint = priv->can.data_bittiming.sample_point,
						  .idealTqNum = 0,
						  .syncJumpWidth = priv->can.data_bittiming.sjw}};
	int err;

	if (priv->can.bittiming.bitrate != 0) {
		timingConf.timingNominal.idealTqNum = CANGURU_CAN_CLK / prescaler
											  / priv->can.bittiming.bitrate;
	}
	if (priv->can.data_bittiming.bitrate != 0) {
		timingConf.timingData.idealTqNum = CANGURU_CAN_CLK / prescalerData
										   / priv->can.data_bittiming.bitrate;
	}
	err = canguru_set_can_conf(priv, &conf);
	if (err != 0) {
		dev_warn(priv->guru_dev->dev, "Unable to set config CAN bus");
		return err;
	}

	err = canguru_set_can_timing(priv, &timingConf);
	if (err != 0) {
		dev_warn(priv->guru_dev->dev, "Unable to set timing CAN bus");
		return err;
	}
	
	return 0;
}

static int canguru_set_can_conf(struct canguru_priv *priv, const struct can_conf_msg *value)
{
	int err;

	err = guru_send_data(priv->guru_dev,
							CAN_MSG_CONFIG_SET,
							(u8 *)value,
							sizeof(struct can_conf_msg));
	if (err != 0) {
		return err;
	}

	return 0;
}

static int canguru_set_can_timing(struct canguru_priv *priv, const struct can_timing_msg *value)
{
	int err;

	err = guru_send_data(priv->guru_dev,
							CAN_MSG_TIMING_SET,
							(u8 *)value,
							sizeof(struct can_timing_msg));
	if (err != 0) {
		return err;
	}
	
	return 0;
}

static int canguru_set_can_enable(struct canguru_priv *priv, bool enable)
{
	const struct can_enable_msg msg = {.iface = priv->channel_idx, .enable = enable};

	return guru_send_data(priv->guru_dev,
							 CAN_MSG_ENABLE_SET,
							 (u8 *)&msg,
							 sizeof(struct can_enable_msg));
}

static int canguru_set_can_ter(struct canguru_priv *priv, bool enable)
{
	const struct can_enable_msg msg = {.iface = priv->channel_idx, .enable = enable};
	
	return guru_send_data(priv->guru_dev,
							 CAN_MSG_TER_ENABLE,
							 (u8 *)&msg,
							 sizeof(struct can_enable_msg));
}

static int canguru_can_reset(struct canguru_priv *priv)
{
	return guru_send_data(priv->guru_dev, CAN_MSG_RESET, &priv->channel_idx, sizeof(u8));
}

static int canguru_set_global_filter(struct canguru_priv *priv)
{
	const struct can_global_filter_msg msg = {.iface = priv->channel_idx,
											  .stdDefaultAllow = true,
											  .extDefaultAllow = true,
											  .stdRemoteReject = false,
											  .extRemoteReject = false};
	
	return guru_send_data(priv->guru_dev,
							 CAN_MSG_GLOBAL_FILTER_SET,
							 (u8 *)&msg,
							 sizeof(struct can_global_filter_msg));
}

static int canguru_can_fd_send(struct canguru_priv *priv, struct canfd_frame *frame)
{
	struct can_send_msg msg = {0};

	msg.send_header.iface = priv->channel_idx;
	msg.send_header.can_header.canfd = true;
	msg.send_header.can_header.esi = (frame->flags & CANFD_ESI) != 0;
	msg.send_header.can_header.bitrate_switch = (frame->flags & CANFD_BRS) != 0;
	msg.send_header.can_header.can_dlc = can_fd_len2dlc(frame->len);

	msg.send_header.can_header.remote_frame = (frame->can_id & CAN_RTR_FLAG) != 0;
	if ((frame->can_id & CAN_EFF_FLAG) != 0) {
		msg.send_header.can_header.extended_id = true;
		msg.send_header.can_header.can_id = frame->can_id & CAN_EXT_ID_MASK;
	} else {
		msg.send_header.can_header.can_id = frame->can_id & CAN_STD_ID_MASK;
	}

	memcpy(msg.data, frame->data, frame->len);
	return guru_send_data(priv->guru_dev,
							 CAN_MSG_SEND,
							 (const uint8_t *)&msg,
							 sizeof(struct can_send_header) + frame->len);
}

static int canguru_can_classic_send(struct canguru_priv *priv, struct can_frame *frame)
{
	struct can_send_msg msg = {0};

	msg.send_header.iface = priv->channel_idx;
	msg.send_header.can_header.can_dlc = can_get_cc_dlc(frame, priv->can.ctrlmode);

	msg.send_header.can_header.remote_frame = (frame->can_id & CAN_RTR_FLAG) != 0;
	if ((frame->can_id & CAN_EFF_FLAG) != 0) {
		msg.send_header.can_header.extended_id = true;
		msg.send_header.can_header.can_id = frame->can_id & CAN_EXT_ID_MASK;
	} else {
		msg.send_header.can_header.can_id = frame->can_id & CAN_STD_ID_MASK;
	}

	memcpy(msg.data, frame->data, frame->len);
	return guru_send_data(priv->guru_dev,
							 CAN_MSG_SEND,
							 (const uint8_t *)&msg,
							 sizeof(struct can_send_header) + frame->len);
}

static void canguru_send_callback(struct canguru_priv *priv, enum guru_status status)
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
