#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define BEEP_CNT 	1
#define BEEP_NAME	"beep"
#define LEDOFF	0
#define LEDON	1

struct beep_dev{
	dev_t	devid;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	int major;
	int minor;
	struct device_node *nd;
	int led_gpio;
};

struct beep_dev beep;

static int led_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &beep;
	return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue;
	unsigned char databuf[1];
	unsigned char ledstat;
	struct beep_dev *dev = filp->private_data;

	retvalue = copy_from_user(databuf, buf, cnt);
	if (retvalue < 0)
	{
		printk("kernel write failed!\r\n");
		return -EFAULT;
	}

	ledstat = databuf[0];

	if(ledstat == LEDON){
		gpio_set_value(dev->led_gpio, 0);
	}else if (ledstat == LEDOFF)
	{
		gpio_set_value(dev->led_gpio, 1);
	}

	return 0;
}

static struct file_operations beep_fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.release = led_release,
	.read = led_read,
	.write = led_write,
};

static int __init led_init(void)
{
	int ret = 0;

	beep.nd = of_find_node_by_path("/beep");
	if (beep.nd == NULL)
	{
		printk("beep node can not found!\r\n");
		return -EINVAL;
	}else {
		printk("beep node has been found!\r\n");
	}

	beep.led_gpio = of_get_named_gpio(beep.nd, "led-gpio", 0);
	if (beep.led_gpio < 0)
	{
		printk("can't get led-gpior\\n");
		return -EINVAL;
	}
	printk("led-gpio num = %dr\n", beep.led_gpio);

	ret = gpio_direction_output(beep.led_gpio, 1);
	if (ret < 0)
	{
		printk("can't set gpio!");
	}
	
	if (beep.major)
	{
		beep.devid = MKDEV(beep.major, 0);
		register_chrdev_region(beep.devid, BEEP_CNT, BEEP_NAME);
	}else{
		alloc_chrdev_region(&beep.devid, 0, BEEP_CNT, BEEP_NAME);
		beep.major = MAJOR(beep.devid);
		beep.minor = MINOR(beep.devid);
	}
	printk("beep major=%d,minor=%d\r\n",beep.major, beep.minor);	
	
	beep.cdev.owner = THIS_MODULE;
	cdev_init(&beep.cdev, &beep_fops);

	cdev_add(&beep.cdev, beep.devid, BEEP_CNT);

	beep.class = class_create(THIS_MODULE, BEEP_NAME);
	if (IS_ERR(beep.class))
	{
		return PTR_ERR(beep.class);
	}

	beep.device = device_create(beep.class, NULL, beep.devid, NULL, BEEP_NAME);
	if (IS_ERR(beep.device))
	{
		return PTR_ERR(beep.device);
	}

	return 0;
	
}

static void __exit led_exit(void)
{
	/* 注销字符设备驱动 */
	cdev_del(&beep.cdev);/*  删除cdev */
	unregister_chrdev_region(beep.devid, BEEP_CNT); /* 注销设备号 */

	device_destroy(beep.class, beep.devid);
	class_destroy(beep.class);
}


module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("rodney");