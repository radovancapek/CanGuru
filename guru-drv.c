#include "guru-device.h"
#include "canguru-device.h"
#include "linguru-device.h"
#include "mbus-device.h"

/* vendor and product id */
#define GURU_MODULE_NAME "oakrey-guru"
#define GURU_VENDOR_ID 0x1fc9
#define GURU_PRODUCT_ID 0x008A

#define CANGURU_PRODUCT_NAME "CAN Guru Lite"
#define LINGURU_PRUDUCT_NAME "LIN Guru"
#define BUSKIT_MUTLIBUS_PRODUCT_NAME "BusKit MultiBus"

static struct class *guru_class;

static const struct usb_device_id guru_usb_table[] = {
	{ USB_DEVICE(GURU_VENDOR_ID, GURU_PRODUCT_ID) },
	{ } /* Terminating entry */
};

MODULE_DEVICE_TABLE(usb, guru_usb_table);

static inline void guru_usb_report(struct usb_device *usbdev)
{
	dev_info(&usbdev->dev,
			 "Starting %s %s (Serial Number %s) driver version %s\n",
			 usbdev->manufacturer,
			 usbdev->product,
			 usbdev->serial,
			 GURU_GIT_VERSION);
}


static inline int guru_usb_device_alloc(struct usb_interface *intf)
{
	int err = -ENODEV;
	void *guru_dev;
	struct usb_device *usbdev = interface_to_usbdev(intf);
	char buf[GURU_STR_LEN_MAX];

	if (usb_string(usbdev, usbdev->descriptor.iProduct, buf, sizeof(buf)) <= 0)
		return err;

	if (strcmp(buf, CANGURU_PRODUCT_NAME) == 0) {
		guru_usb_report(usbdev);
		guru_dev =  devm_kzalloc(&intf->dev, sizeof(struct canguru_device), GFP_KERNEL);
		if (guru_dev == NULL) {
			return -ENOMEM;
		}
		err = canguru_dev_init((struct canguru_device *)guru_dev, intf, guru_class);
	} else if (strcmp(buf, LINGURU_PRUDUCT_NAME) == 0) {
		guru_usb_report(usbdev);
		guru_dev = devm_kzalloc(&intf->dev, sizeof(struct linguru_device), GFP_KERNEL);
		if (guru_dev == NULL) {
			return -ENOMEM;
		}
		err = linguru_dev_init((struct linguru_device *)guru_dev, intf, guru_class);
	} else if (strcmp(buf, BUSKIT_MUTLIBUS_PRODUCT_NAME) == 0) {
		guru_usb_report(usbdev);
		guru_dev =  devm_kzalloc(&intf->dev, sizeof(struct mbus_device), GFP_KERNEL);
		if (guru_dev == NULL) {
			return -ENOMEM;
		}
		err = mbus_dev_init((struct mbus_device *)guru_dev, intf, guru_class);
	} else {
		dev_info(&usbdev->dev, "Device ignored, not an Guru device\n");
	}

	return err;
}

static int guru_usb_probe(struct usb_interface *intf,
							 __maybe_unused const struct usb_device_id *id)
{
	int err = guru_usb_device_alloc(intf);
	
	return err;
}

static void guru_usb_disconnect(__maybe_unused struct usb_interface *intf)
{
	struct guru_device *guru_dev = usb_get_intfdata(intf);

	guru_dev_deinit(guru_dev);
}

static struct usb_driver guru_usb_driver = {
	.name = GURU_MODULE_NAME,
	.probe = guru_usb_probe,
	.disconnect = guru_usb_disconnect,
	.id_table =	guru_usb_table,
};

static int __init guru_drv_init(void)
{
	int err;
	
	guru_class = class_create(THIS_MODULE, GURU_MODULE_NAME);
	if (IS_ERR(guru_class)) {
		printk(KERN_WARNING GURU_MODULE_NAME ": GURU class initialization failed\n");
		err = PTR_ERR(guru_class);
		return err;
	}
	
	err = guru_boot_init();
	if (err != 0) {
		printk(KERN_WARNING GURU_MODULE_NAME ": GURU boot initialization failed\n");
		goto err_boot;
	}
	
	err = usb_register(&guru_usb_driver);
	if (err != 0) {
		printk(KERN_WARNING GURU_MODULE_NAME ": USB module initialization failed\n");
		goto err_usb;
	}
	
	return 0;

err_boot:
	class_unregister(guru_class);
	class_destroy(guru_class);

err_usb:
	guru_boot_exit();
	return err;
}

static void __exit guru_drv_exit(void)
{
	usb_deregister(&guru_usb_driver);
	guru_boot_exit();
	class_destroy(guru_class);
}

module_init(guru_drv_init);
module_exit(guru_drv_exit);

MODULE_AUTHOR("Michal Kona <kona@oakrey.cz>");
MODULE_DESCRIPTION("SocketCAN driver for Oakrey Guru devices");
MODULE_VERSION(GURU_GIT_VERSION);
MODULE_LICENSE("GPL v2");
