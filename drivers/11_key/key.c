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
#include <linux/semaphore.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define KEY_CNT		1
#define KEY_NAME	"key"

#define KEY0VALUE	0xF0
#define INVAKEY		0x00

struct key_dev
{
	dev_t devid;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	int major;
	int minor;
	struct device_node *nd;
	int key_gpio;
	atomic_t keyvalue;
};

struct key_dev keydev;

static int keyio_init(void)
{
	keydev.nd = of_find_node_by_path("/key");
	if (keydev.nd == NULL)
	{
		return -EINVAL;
	}
	
	keydev.key_gpio = of_get_named_gpio(keydev.nd, "key-gpio", 0);
	if (keydev.key_gpio < 0)
	{
		printk("can't get key0\r\n");
		return -EINVAL;
	}
	printk("key_gpio = %d\r\n", keydev.key_gpio);
	
	gpio_request(keydev.key_gpio, "key0");
	gpio_direction_input(keydev.key_gpio);
	return 0;
}

static int key_open(struct inode *inode, struct file *filp){
	int ret = 0;
	filp->private_data = &keydev;

	ret = keyio_init();
	if (ret <0 )
	{
		return ret;
	}
	return 0;
}

static ssize_t key_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	int ret = 0;
	int value;
	struct key_dev *dev = filp->private_data;

	if (gpio_get_value(dev->key_gpio)  == 0)	
	{
		while (!gpio_get_value(dev->key_gpio));
		atomic_set(&dev->keyvalue, KEY0VALUE);
	}else{
		atomic_set(&dev->keyvalue, INVAKEY);
	}

	value = atomic_read(&dev->keyvalue);
	ret = copy_to_user(buf, &value, sizeof(value));
	return ret;
}

static ssize_t key_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	return 0;
}

static int key_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static struct file_operations key_fops = {
	.owner = THIS_MODULE,
	.open = key_open,
	.read = key_read,
	.write = key_write,
	.release = 	key_release,
};

/*
 * @description	: 驱动入口函数
 * @param 		: 无
 * @return 		: 无
 */
static int __init mykey_init(void)
{
	/* 初始化原子变量 */
	atomic_set(&keydev.keyvalue, INVAKEY);

	/* 注册字符设备驱动 */
	/* 1、创建设备号 */
	if (keydev.major) {		/*  定义了设备号 */
		keydev.devid = MKDEV(keydev.major, 0);
		register_chrdev_region(keydev.devid, KEY_CNT, KEY_NAME);
	} else {						/* 没有定义设备号 */
		alloc_chrdev_region(&keydev.devid, 0, KEY_CNT, KEY_NAME);	/* 申请设备号 */
		keydev.major = MAJOR(keydev.devid);	/* 获取分配号的主设备号 */
		keydev.minor = MINOR(keydev.devid);	/* 获取分配号的次设备号 */
	}
	
	/* 2、初始化cdev */
	keydev.cdev.owner = THIS_MODULE;
	cdev_init(&keydev.cdev, &key_fops);
	
	/* 3、添加一个cdev */
	cdev_add(&keydev.cdev, keydev.devid, KEY_CNT);

	/* 4、创建类 */
	keydev.class = class_create(THIS_MODULE, KEY_NAME);
	if (IS_ERR(keydev.class)) {
		return PTR_ERR(keydev.class);
	}

	/* 5、创建设备 */
	keydev.device = device_create(keydev.class, NULL, keydev.devid, NULL, KEY_NAME);
	if (IS_ERR(keydev.device)) {
		return PTR_ERR(keydev.device);
	}
	
	return 0;
}

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static void __exit mykey_exit(void)
{
	/* 注销字符设备驱动 */
	gpio_free(keydev.key_gpio);
	cdev_del(&keydev.cdev);/*  删除cdev */
	unregister_chrdev_region(keydev.devid, KEY_CNT); /* 注销设备号 */

	device_destroy(keydev.class, keydev.devid);
	class_destroy(keydev.class);
}

module_init(mykey_init);
module_exit(mykey_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("rodney");
