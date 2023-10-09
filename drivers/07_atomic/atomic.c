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

#define GPIOLED_CNT 	1
#define GPIOLED_NAME	"gpioled"
#define LEDOFF	0
#define LEDON	1

struct gpioled_dev{
	dev_t	devid;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	int major;
	int minor;
	struct device_node *nd;
	int led_gpio;
	atomic_t lock;
};

struct gpioled_dev gpioled;

static int led_open(struct inode *inode, struct file *filp)
{
	// filp->private_data = &gpioled;
	// return 0;

	//通过判断原子变量的值来检查LED有没有被别的应用使用
	if (!atomic_dec_and_test(&gpioled.lock))
	{
		atomic_inc(&gpioled.lock);
		return -EBUSY;
	}
	filp->private_data = &gpioled;
	return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
	struct gpioled_dev *dev = filp->private_data;

	atomic_inc(&dev->lock);
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
	struct gpioled_dev *dev = filp->private_data;

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

static struct file_operations gpioled_fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.release = led_release,
	.read = led_read,
	.write = led_write,
};

static int __init led_init(void)
{
	int ret = 0;

	atomic_set(&gpioled.lock, 1);

	gpioled.nd = of_find_node_by_path("/gpioled");
	if (gpioled.nd == NULL)
	{
		printk("gpioled node can not found!\r\n");
		return -EINVAL;
	}else {
		printk("gpioled node has been found!\r\n");
	}

	gpioled.led_gpio = of_get_named_gpio(gpioled.nd, "led-gpio", 0);
	if (gpioled.led_gpio < 0)
	{
		printk("can't get led-gpior\\n");
		return -EINVAL;
	}
	printk("led-gpio num = %dr\n", gpioled.led_gpio);

	ret = gpio_direction_output(gpioled.led_gpio, 1);
	if (ret < 0)
	{
		printk("can't set gpio!");
	}
	
	if (gpioled.major)
	{
		gpioled.devid = MKDEV(gpioled.major, 0);
		register_chrdev_region(gpioled.devid, GPIOLED_CNT, GPIOLED_NAME);
	}else{
		alloc_chrdev_region(&gpioled.devid, 0, GPIOLED_CNT, GPIOLED_NAME);
		gpioled.major = MAJOR(gpioled.devid);
		gpioled.minor = MINOR(gpioled.devid);
	}
	printk("gpioled major=%d,minor=%d\r\n",gpioled.major, gpioled.minor);	
	
	gpioled.cdev.owner = THIS_MODULE;
	cdev_init(&gpioled.cdev, &gpioled_fops);

	cdev_add(&gpioled.cdev, gpioled.devid, GPIOLED_CNT);

	gpioled.class = class_create(THIS_MODULE, GPIOLED_NAME);
	if (IS_ERR(gpioled.class))
	{
		return PTR_ERR(gpioled.class);
	}

	gpioled.device = device_create(gpioled.class, NULL, gpioled.devid, NULL, GPIOLED_NAME);
	if (IS_ERR(gpioled.device))
	{
		return PTR_ERR(gpioled.device);
	}

	return 0;
	
}

static void __exit led_exit(void)
{
	/* 注销字符设备驱动 */
	cdev_del(&gpioled.cdev);/*  删除cdev */
	unregister_chrdev_region(gpioled.devid, GPIOLED_CNT); /* 注销设备号 */

	device_destroy(gpioled.class, gpioled.devid);
	class_destroy(gpioled.class);
}


module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("rodney");