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
#include <linux/platform_device.h>
#include <linux/miscdevice.h>

#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define MISCBEEP_NAME "miscbeep"
#define MISCBEEP_MINOR 144
#define BEEPOFF	0
#define BEEPON	1

struct miscbeep_dev{
	dev_t devid;
	struct cdev dev;
	struct class *class;
	struct device *device;
	struct device_node *nd;
	int beep_gpio;
};

struct miscbeep_dev miscbeep;

static int miscbeep_open(struct inode *inode, struct file *filp){
	filp->private_data = &miscbeep;
	return 0;
}

static ssize_t miscbeep_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt){
	int retvalue;
	unsigned char databuf[1];
	unsigned char beepstat;
	struct miscbeep_dev *dev = filp->private_data;

	retvalue = copy_from_user(databuf, buf, cnt);
	if (retvalue < 0)
	{
		printk("kernel write failed!\r\n");
		return -EFAULT;
	}

	beepstat = databuf[0];
	if (beepstat == BEEPON)
	{
		gpio_set_value(dev->beep_gpio, 0);
	}else if (beepstat == BEEPOFF)
	{
		gpio_set_value(dev->beep_gpio, 1);
	}

	return 0;
}

static struct file_operations miscbeep_fops = {
	.owner = THIS_MODULE,
	.open = miscbeep_open,
	.write = miscbeep_write,
};

//MISC设备结构体
static struct miscdevice beep_miscdev = {
	.minor = MISCBEEP_MINOR,
	.name = MISCBEEP_NAME,
	.fops = &miscbeep_fops,
};

static int miscbeep_probe(struct platform_device *dev)
{
	int ret = 0;

	printk("beep driver and device was matched!\r\n");

	miscbeep.nd = of_find_node_by_path("/beep");
	if (miscbeep.nd == NULL)
	{
		printk("beep node not find!\r\n");
		return -EINVAL;
	}

	miscbeep.beep_gpio = of_get_named_gpio(miscbeep.nd, "beep-gpio", 0);
	if (miscbeep.beep_gpio < 0)
	{
		printk("can't get beep-gpio!\r\n");
		return -EINVAL;
	}

	gpio_request(miscbeep.beep_gpio, "beep");
	ret = gpio_direction_output(miscbeep.beep_gpio, 1);
	if (ret < 0)
	{
		printk("can't set gpio!\r\n");
	}

	//注册MISC设备
	ret = misc_register(&beep_miscdev);
	if (ret < 0)
	{
		printk("misc device register failed!\r\n");
		return -EFAULT;
	}

	return 0;
}

static int miscbeep_remove(struct platform_device *dev){
	gpio_set_value(miscbeep.beep_gpio, 1);
	gpio_free(miscbeep.beep_gpio);

	misc_deregister(&beep_miscdev);
	return 0;
}

static const struct of_device_id beep_of_match[] = {
	{ .compatible = "atkalpha-beep" },
	{	}
};

static struct platform_driver beep_driver = {
	.driver = {
		.name = "imx6ul-beep",
		.of_match_table = beep_of_match,
	},
	.probe = miscbeep_probe,
	.remove = miscbeep_remove,
};

static int __init miscbeep_init(void){
	return platform_driver_register(&beep_driver);
}

static void __exit miscbeep_exit(void){
	platform_driver_unregister(&beep_driver);
}

module_init(miscbeep_init);
module_exit(miscbeep_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("rodney");
