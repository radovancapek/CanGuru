#include "linguru-voltage.h"

#include "guru-device.h"
#include "linguru-msg-net.h"

#include <linux/iio/sysfs.h>
#include <linux/iio/events.h>

#define LINGURU_VOLT_TIMEOUT (msecs_to_jiffies(100))

static const struct iio_chan_spec lin_volt_channels[] = {
	{.type = IIO_VOLTAGE,
	 .indexed = 1,
	 .channel = 0,
	 .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
	 .datasheet_name = "lin"}};

static int linguru_volt_read_raw(
	struct iio_dev *indio_dev, struct iio_chan_spec const *chan, int *val, int *val2, long mask);

static const struct iio_info lin_volt_info = {
	.read_raw = &linguru_volt_read_raw,
};

int linguru_volt_create(struct iio_dev **new_dev, const struct linguru_voltage_conf *conf)
{
	struct iio_dev *indio_dev = NULL;
	struct linguru_voltage_priv *priv;
	int err;

	indio_dev = iio_device_alloc(conf->guru_dev->dev, sizeof(struct linguru_voltage_priv));
	if (indio_dev == NULL) {
		dev_err(conf->guru_dev->dev, "Couldn't alloc iiodev\n");
		return -ENOMEM;
	}

	iio_device_set_parent(indio_dev, conf->guru_dev->dev);
	priv = iio_priv(indio_dev);
	priv->guru_dev = conf->guru_dev;
	priv->voltage = 0;
	mutex_init(&priv->lock);
	init_completion(&priv->completion);

	indio_dev->name = dev_name(conf->guru_dev->dev);
	indio_dev->info = &lin_volt_info;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = lin_volt_channels;
	indio_dev->num_channels = ARRAY_SIZE(lin_volt_channels);

	err = iio_device_register(indio_dev);
	if (err != 0) {
		return err;
	}
	*new_dev = indio_dev;
	return 0;
}

void linguru_volt_destroy(struct iio_dev **indio_dev)
{
	iio_device_unregister(*indio_dev);
	iio_device_free(*indio_dev);
	*indio_dev = NULL;
}

void linguru_volt_reply(struct iio_dev *indio_dev, u32 voltage)
{
	struct linguru_voltage_priv *priv = iio_priv(indio_dev);

	priv->voltage = voltage;
	complete(&priv->completion);
}

static int linguru_volt_read_raw(struct iio_dev *indio_dev,
								 __maybe_unused struct iio_chan_spec const *chan,
								 int *val,
								 int *val2,
								 long mask)
{
	unsigned long timeout;
	struct linguru_voltage_priv *priv = iio_priv(indio_dev);
	int err;

	if (mask != IIO_CHAN_INFO_RAW) {
		return -EINVAL;
	}
	
	mutex_lock(&priv->lock);
	reinit_completion(&priv->completion);

	err = guru_send_data(priv->guru_dev, LIN_MSG_VOLT_GET, NULL, 0);
	if (err != 0) {
		mutex_unlock(&priv->lock);
		return err;
	}

	timeout = wait_for_completion_timeout(&priv->completion, LINGURU_VOLT_TIMEOUT);
	if (timeout == 0) {
		dev_warn(priv->guru_dev->dev, "LinGuru get volt timed out\n");
		err = -ETIMEDOUT;
	} else {
		*val = priv->voltage;
		*val2 = 0;
		err = IIO_VAL_INT;
	}
	
	mutex_unlock(&priv->lock);
	return err;
}
