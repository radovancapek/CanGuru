#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xf704969, "module_layout" },
	{ 0xc946dda0, "cdev_del" },
	{ 0x2d725fd4, "cdev_init" },
	{ 0x72f94297, "register_candev" },
	{ 0x619cb7dd, "simple_read_from_buffer" },
	{ 0x754d539c, "strlen" },
	{ 0x54b1fac6, "__ubsan_handle_load_invalid_value" },
	{ 0xebf6213f, "no_llseek" },
	{ 0x82e7bb9c, "device_destroy" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0xbb50aa3a, "iio_device_unregister" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x9d8a4817, "usb_unanchor_urb" },
	{ 0x71ac5e55, "netif_rx" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xdd83e58e, "netif_schedule_queue" },
	{ 0x8193f9be, "_dev_warn" },
	{ 0xdb4ebb88, "close_candev" },
	{ 0x1eaf2b5b, "usb_string" },
	{ 0x9898d6aa, "netif_tx_wake_queue" },
	{ 0xb7f634e0, "usb_deregister" },
	{ 0xcefb0c9f, "__mutex_init" },
	{ 0xcdef5c0e, "class_unregister" },
	{ 0xa00aca2a, "dql_completed" },
	{ 0xf12d9387, "can_fd_dlc2len" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0xdbd16536, "kfree_skb_reason" },
	{ 0xf5e4484, "__iio_device_register" },
	{ 0x6047ede6, "can_fd_len2dlc" },
	{ 0xefc94da8, "device_create" },
	{ 0x882a1d87, "alloc_candev_mqs" },
	{ 0xa66f0b1b, "free_candev" },
	{ 0xea7b7d10, "usb_free_coherent" },
	{ 0x461dee21, "_dev_err" },
	{ 0xc378cea7, "cdev_add" },
	{ 0x87a21cb3, "__ubsan_handle_out_of_bounds" },
	{ 0x55dde90f, "_dev_info" },
	{ 0x9a536a95, "usb_submit_urb" },
	{ 0x6383b27c, "__x86_indirect_thunk_rdx" },
	{ 0xc865cafc, "unregister_candev" },
	{ 0x962c8ae1, "usb_kill_anchored_urbs" },
	{ 0x463cb585, "alloc_can_skb" },
	{ 0xfe0db602, "iio_device_free" },
	{ 0xd0da656b, "__stack_chk_fail" },
	{ 0x92997ed8, "_printk" },
	{ 0x310802f2, "iio_device_alloc" },
	{ 0x65487097, "__x86_indirect_thunk_rax" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0xb0bf5c9b, "open_candev" },
	{ 0x4740e91c, "can_skb_get_frame_len" },
	{ 0x8f63b77a, "usb_register_driver" },
	{ 0x933c4a18, "class_destroy" },
	{ 0x608741b5, "__init_swait_queue_head" },
	{ 0xd559c565, "alloc_canfd_skb" },
	{ 0xa6257a2f, "complete" },
	{ 0xc7fdd378, "can_get_echo_skb" },
	{ 0xe1b5d8df, "usb_alloc_coherent" },
	{ 0xf3388fae, "can_put_echo_skb" },
	{ 0x6a118319, "devm_kmalloc" },
	{ 0x4a3ad70e, "wait_for_completion_timeout" },
	{ 0x325cb5cb, "__class_create" },
	{ 0x41882bd9, "usb_free_urb" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xca7779cf, "usb_anchor_urb" },
	{ 0x249a9ef3, "usb_alloc_urb" },
};

MODULE_INFO(depends, "can-dev,industrialio");

MODULE_ALIAS("usb:v1FC9p008Ad*dc*dsc*dp*ic*isc*ip*in*");

MODULE_INFO(srcversion, "DB434867E65933223DDB1A6");
