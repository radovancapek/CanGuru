#ifndef GURU_BOOT_H
#define GURU_BOOT_H

#include <linux/cdev.h>

#define GURU_SN_SIZE 16
#define GURU_SN_END_SIZE 2

struct guru_device;
struct class;
struct device;

struct guru_boot_conf {
	struct guru_device *guru_dev;
};

struct guru_boot {
	struct guru_device *guru_dev;
	u32 minor_num;
	struct device *boot_device;
	struct cdev boot_cdev;
	char serial_number[GURU_SN_SIZE + GURU_SN_END_SIZE];
};

int guru_boot_init(void);
void guru_boot_exit(void);
int guru_boot_create(struct guru_boot *self, const struct guru_boot_conf *conf);
void guru_boot_destroy(struct guru_boot *self);

#endif // GURU_BOOT_H
