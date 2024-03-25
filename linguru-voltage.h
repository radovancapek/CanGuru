#ifndef LINGURU_VOLTAGE_H
#define LINGURU_VOLTAGE_H

#include <linux/iio/iio.h>

struct guru_device;

struct linguru_voltage_conf
{
	struct guru_device *guru_dev;
};

struct linguru_voltage_priv
{
	struct guru_device *guru_dev;

	struct completion completion;
	struct mutex lock;
	uint32_t voltage;
};

int linguru_volt_create(struct iio_dev **new_dev, const struct linguru_voltage_conf *conf);
void linguru_volt_destroy(struct iio_dev **indio_dev);
void linguru_volt_reply(struct iio_dev *indio_dev, u32 voltage);

#endif // LINGURU_VOLTAGE_H
