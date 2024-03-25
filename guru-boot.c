#include "guru-boot.h"

#include <linux/fs.h>

#include "guru-device.h"

#define GURU_BOOT_DEV_NAME "guru"

static int guru_sn_init(struct guru_boot *self);
static int guru_boot_open(struct inode *inode, struct file *file);
static ssize_t guru_boot_read(struct file *file, char __user *buff, size_t length, loff_t *offset);
static long guru_boot_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static struct file_operations guru_boot_fops = {.owner = THIS_MODULE,
												.open = guru_boot_open,
												.read = guru_boot_read,
												.unlocked_ioctl = guru_boot_ioctl,
												.llseek = no_llseek};

static dev_t guru_boot_id;
static struct guru_boot *guru_boot_table[GURU_DEV_MAX];

/* Guru bootloader public methods implementation */

int guru_boot_init(void)
{
	int err;

	err = alloc_chrdev_region(&guru_boot_id, 0, GURU_DEV_MAX, GURU_BOOT_DEV_NAME);
	if (err != 0) {
		return err;
	}

	return 0;
}

void guru_boot_exit(void)
{
	unregister_chrdev_region(guru_boot_id, GURU_DEV_MAX);
}

int guru_boot_create(struct guru_boot *self, const struct guru_boot_conf *conf)
{
	int err;
	int idx;

	for (idx = 0; idx < GURU_DEV_MAX; idx++) {
		if (guru_boot_table[idx] == NULL) {
			self->minor_num = idx;
			break;
		}

		if (idx >= (GURU_DEV_MAX - 1)) {
			return -ENOMEM;
		}
	}

	cdev_init(&self->boot_cdev, &guru_boot_fops);
	err = cdev_add(&self->boot_cdev, guru_boot_id, 1);
	if (err != 0) {
		dev_err(conf->guru_dev->dev, "Unable to add guru boot cdev.");
		return err;
	}

	self->boot_device = device_create(conf->guru_dev->guru_class,
									  conf->guru_dev->dev,
									  MKDEV(MAJOR(guru_boot_id), self->minor_num),
									  self,
									  "%s%d",
									  GURU_BOOT_DEV_NAME,
									  self->minor_num);
	if (IS_ERR(self->boot_device)) {
		dev_err(conf->guru_dev->dev, "Unable to create guru boot device.");
		err = PTR_ERR(self->boot_device);
		cdev_del(&self->boot_cdev);
		return err;
	}
	self->guru_dev = conf->guru_dev;
	guru_boot_table[self->minor_num] = self;
	
	err = guru_sn_init(self);
	if (err != 0) {
		dev_err(conf->guru_dev->dev, "Invalid guru serial number.");
		guru_boot_destroy(self);
		return err;
	}

	return 0;
}

void guru_boot_destroy(struct guru_boot *self)
{
	device_destroy(self->guru_dev->guru_class, MKDEV(MAJOR(guru_boot_id), self->minor_num));
	guru_boot_table[self->minor_num] = NULL;
	cdev_del(&self->boot_cdev);
}

/* Guru bootloader private methods implementation */

static int guru_sn_init(struct guru_boot *self)
{
	size_t sn_size = strlen(self->guru_dev->usbdev->serial);

	if (sn_size != GURU_SN_SIZE) {
		return -EINVAL;
	}
	sprintf(self->serial_number, "%s\n", self->guru_dev->usbdev->serial);

	return 0;
}

static int guru_boot_open(struct inode *inode, struct file *file)
{
	struct guru_boot *priv = container_of(inode->i_cdev, struct guru_boot, boot_cdev);

	file->private_data = priv;
	return 0;
}

static ssize_t guru_boot_read(struct file *file,
							  char __user *buff,
							  size_t length,
							  loff_t *offset)
{
	__maybe_unused struct guru_boot *priv = file->private_data;
	
	return 	simple_read_from_buffer(buff, length, offset,
								   priv->serial_number,
								   strlen(priv->serial_number));
}

static long guru_boot_ioctl(__maybe_unused struct file *file,
							__maybe_unused unsigned int cmd,
							__maybe_unused unsigned long arg)
{
	return 0;
}
