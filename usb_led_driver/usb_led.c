/*************************************************************************
 *  copy from https://github.com/Qunero/dnw4linux.git
 *
 *  Author: slash.huang <slash.linux.c@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 **************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define DRIVER_NAME		"usb_led"
#define BULKOUT_BUFFER_SIZE	512
#define EP_CTRL			0
#define REQ_BUF_CNT		10
#define USB_LED_PID		0x1111
#define USB_LED_VID		0x5555
#define REQ_ACK			0x00	/* RA6M3 device : ack */
#define REQ_GET_VENDOR		0x02	/* RA6M3 DEV : GET_VENDOR */
#define REQ_SET_VENDOR		0x01 	/* RA6M3 DEV : SET_VENDOR */
#define MSG_TIMEOUT		(3 * HZ)
static struct usb_driver usb_led_driver;
static struct usb_class_driver usb_led_class;

struct usb_led_dev
{
	struct usb_device *udev;
	struct mutex io_mutex;
	char	*bulkout_buffer;
	__u8	bulk_out_endpointAddr;
};

static struct usb_device_id usb_led_table[]= {
	{ USB_DEVICE(USB_LED_PID, USB_LED_VID)},
	{ }
};

static void usb_led_disconnect(struct usb_interface *interface)
{
	struct usb_led_dev *dev = NULL;

	dev = usb_get_intfdata(interface);
	if (NULL != dev)
		kfree(dev);

	usb_deregister_dev(interface, &usb_led_class);
}

static ssize_t usb_led_read(struct file *file, char __user *buf, size_t len, loff_t *loff)
{
	struct usb_led_dev *dev = file->private_data;
	unsigned char req_buf[REQ_BUF_CNT];
	int ret;

	ret = usb_control_msg_send(dev->udev,
				EP_CTRL,
				REQ_SET_VENDOR,
				USB_TYPE_VENDOR | USB_DIR_OUT | USB_RECIP_INTERFACE,
				0,
				0,
				req_buf,
				REQ_BUF_CNT,
				MSG_TIMEOUT,
				GFP_KERNEL);
	if (ret) {
		pr_err("usb_control_msg_send ret %d\n", ret);
		return -EFAULT;
	}

	return -EPERM;
}

static ssize_t usb_led_write(struct file *file, const char __user *buf, size_t len, loff_t *loff)
{
	struct usb_led_dev *dev = file->private_data;
	size_t to_write, total_writed;
	int ret, act_len;
	unsigned char req_buf[REQ_BUF_CNT];

	ret = usb_control_msg_send(dev->udev,
				EP_CTRL,
				REQ_GET_VENDOR,
				USB_TYPE_VENDOR | USB_DIR_IN | USB_RECIP_INTERFACE,
				0,
				0,
				req_buf,
				REQ_BUF_CNT,
				MSG_TIMEOUT,
				GFP_KERNEL);
	if (ret) {
		pr_err("usb_control_msg_send ret %d\n", ret);
		return -EFAULT;
	}

	total_writed = 0;
	while (len > 0) {
		to_write = min(len, (size_t) BULKOUT_BUFFER_SIZE);

		ret = copy_from_user(dev->bulkout_buffer, buf + total_writed, to_write);
		if (ret) {
			pr_err("copy_from_user fail ret %d\n", ret);
			return -EFAULT;
		}

		ret = usb_bulk_msg(dev->udev,
				usb_sndbulkpipe(dev->udev, dev->bulk_out_endpointAddr),
				dev->bulkout_buffer,
				to_write,
				&act_len,
				MSG_TIMEOUT);

		if (ret || (act_len != to_write)) {
			pr_err("usb_bulk_msg fail ret %d\n", ret);
			return -EFAULT;
		}

		len -= to_write;
		total_writed += to_write;
	}

	return total_writed;
}

static int usb_led_open(struct inode *node, struct file *file)
{
	struct usb_interface *interface;
	struct usb_led_dev *dev;

	interface = usb_find_interface(&usb_led_driver, iminor(node));
	if (!interface)
		return -ENODEV;

	dev = usb_get_intfdata(interface);
	dev->bulkout_buffer = kzalloc(BULKOUT_BUFFER_SIZE, GFP_KERNEL);
	if (!(dev->bulkout_buffer))
		return -ENOMEM;

	if (!mutex_trylock(&dev->io_mutex))
		return -EBUSY;

	file->private_data = dev;
	return 0;
}

static int usb_led_release(struct inode *node, struct file *file)
{
	struct usb_led_dev *dev;

	dev = (struct usb_led_dev *)file->private_data;
	kfree(dev->bulkout_buffer);
	mutex_unlock(&dev->io_mutex);
	return 0;
}

static struct file_operations usb_led_fops = {
	.owner   = THIS_MODULE,
	.read    = usb_led_read,
	.write   = usb_led_write,
	.open    = usb_led_open,
	.release = usb_led_release,
};

static struct usb_class_driver usb_led_class = {
	.name = "usb_led%d",
	.fops = &usb_led_fops,
};

static int usb_led_probe(struct usb_interface *interface,
	const struct usb_device_id *id)
{
	struct usb_led_dev *dev;
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	int i, ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		ret = -ENOMEM;
		goto error;
	}

	iface_desc = interface->cur_altsetting;
	for (i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
		endpoint = &(iface_desc->endpoint[i].desc);
		if (!dev->bulk_out_endpointAddr &&
			usb_endpoint_is_bulk_out(endpoint)) {
			pr_info("bulk out endpoint found\n");
			dev->bulk_out_endpointAddr = endpoint->bEndpointAddress;
			break;
		}
	}

	if (!(dev->bulk_out_endpointAddr)) {
		ret = -EBUSY;
		goto error;
	}

	ret = usb_register_dev(interface, &usb_led_class);
	if (ret) {
		pr_err("usb_register_dev fail ret %d\n", ret);
		return ret;
	}

	dev->udev = usb_get_dev(interface_to_usbdev(interface));
	usb_set_intfdata(interface, dev);
	mutex_init(&dev->io_mutex);

	/* do a easy fpr device ack */
	ret = usb_control_msg_send(dev->udev,
				EP_CTRL,
				REQ_ACK,
				USB_TYPE_VENDOR | USB_DIR_OUT | USB_RECIP_INTERFACE,
				0,
				0,
				0,
				0,
				MSG_TIMEOUT,
				GFP_KERNEL);
	if (ret)
		pr_err("usb_control_msg_send fail ret %d\n", ret);

	return 0;
error:
	if (!dev)
		kfree(dev);

	return ret;
}

static struct usb_driver usb_led_driver =
{
	.name       = "usb_led",
	.probe      = usb_led_probe,
	.disconnect = usb_led_disconnect,
	.id_table   = usb_led_table,
	.supports_autosuspend = 1,
};

static int __init usb_led_init(void)
{
	int ret;

	ret = usb_register(&usb_led_driver);
	if (ret) {
		pr_err("usb_register fail\n");
		return ret;
	}
	return 0;
}

static void __exit usb_led_exit(void)
{
	usb_deregister(&usb_led_driver);
}

module_init(usb_led_init);
module_exit(usb_led_exit);
MODULE_AUTHOR("slash.huang <slash.linux.c@gmail.com>");
MODULE_DESCRIPTION("RA6M3 USB LED Driver");
MODULE_LICENSE("GPL");
