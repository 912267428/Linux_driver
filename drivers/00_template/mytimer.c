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
#include <linux/timer.h>
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
/**
 * 使用的名字mytimer
 * 用需要使用的名字替换"mytimer" 大小写均要
 * 
 * 
*/

#define MYTIMER_CNT			1		/* 设备号个数 	*/
#define MYTIMER_NAME		"mytimer"	/* 名字 		*/
#define CLOSE_CMD 		(_IO(0XEF, 0x1))	/* 关闭定时器 */
#define OPEN_CMD		(_IO(0XEF, 0x2))	/* 打开定时器 */
#define SETPERIOD_CMD	(_IO(0XEF, 0x3))	/* 设置定时器周期命令 */
#define LEDON 			1		/* 开灯 */
#define LEDOFF 			0		/* 关灯 */

/* timer设备结构体 */
struct mytimer_dev{
	dev_t devid;			/* 设备号 	 */
	struct cdev cdev;		/* cdev 	*/
	struct class *class;	/* 类 		*/
	struct device *device;	/* 设备 	 */
	int major;				/* 主设备号	  */
	int minor;				/* 次设备号   */
	struct device_node	*nd; /* 设备节点 */
	int led_gpio;			/* key所使用的GPIO编号		*/
	int timeperiod; 		/* 定时周期,单位为ms */
	struct timer_list timer;/* 定义一个定时器*/
	spinlock_t lock;		/* 定义自旋锁 */
};

struct mytimer_dev mytimerdev;	/* timer设备 */

/*
 * @description	: 初始化LED灯IO，open函数打开驱动的时候
 * 				  初始化LED灯所使用的GPIO引脚。
 * @param 		: 无
 * @return 		: 无
 */
static int led_init(void)
{
	int ret = 0;

	mytimerdev.nd = of_find_node_by_path("/gpioled");
	if (mytimerdev.nd== NULL) {
		return -EINVAL;
	}

	mytimerdev.led_gpio = of_get_named_gpio(mytimerdev.nd ,"led-gpio", 0);
	if (mytimerdev.led_gpio < 0) {
		printk("can't get led\r\n");
		return -EINVAL;
	}
	
	/* 初始化led所使用的IO */
	gpio_request(mytimerdev.led_gpio, "led");		/* 请求IO 	*/
	ret = gpio_direction_output(mytimerdev.led_gpio, 1);
	if(ret < 0) {
		printk("can't set gpio!\r\n");
	}
	return 0;
}

/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int mytimer_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	filp->private_data = &mytimerdev;	/* 设置私有数据 */

	mytimerdev.timeperiod = 1000;		/* 默认周期为1s */
	ret = led_init();				/* 初始化LED IO */
	if (ret < 0) {
		return ret;
	}

	return 0;
}

/*
 * @description		: ioctl函数，
 * @param - filp 	: 要打开的设备文件(文件描述符)
 * @param - cmd 	: 应用程序发送过来的命令
 * @param - arg 	: 参数
 * @return 			: 0 成功;其他 失败
 */
static long mytimer_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct mytimer_dev *dev =  (struct mytimer_dev *)filp->private_data;
	int timerperiod;
	unsigned long flags;
	
	switch (cmd) {
		case CLOSE_CMD:		/* 关闭定时器 */
			del_timer_sync(&dev->timer);
			break;
		case OPEN_CMD:		/* 打开定时器 */
			spin_lock_irqsave(&dev->lock, flags);
			timerperiod = dev->timeperiod;
			spin_unlock_irqrestore(&dev->lock, flags);
			mod_timer(&dev->timer, jiffies + msecs_to_jiffies(timerperiod));
			break;
		case SETPERIOD_CMD: /* 设置定时器周期 */
			spin_lock_irqsave(&dev->lock, flags);
			dev->timeperiod = arg;
			spin_unlock_irqrestore(&dev->lock, flags);
			mod_timer(&dev->timer, jiffies + msecs_to_jiffies(arg));
			break;
		default:
			break;
	}
	return 0;
}

/* 设备操作函数 */
static struct file_operations mytimer_fops = {
	.owner = THIS_MODULE,
	.open = mytimer_open,
	.unlocked_ioctl = mytimer_unlocked_ioctl,
};

/* 定时器回调函数 */
void timer_function(unsigned long arg)
{
	struct mytimer_dev *dev = (struct mytimer_dev *)arg;
	static int sta = 1;
	int timerperiod;
	unsigned long flags;

	sta = !sta;		/* 每次都取反，实现LED灯反转 */
	gpio_set_value(dev->led_gpio, sta);
	
	/* 重启定时器 */
	spin_lock_irqsave(&dev->lock, flags);
	timerperiod = dev->timeperiod;
	spin_unlock_irqrestore(&dev->lock, flags);
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(dev->timeperiod)); 
 }

/*
 * @description	: 驱动入口函数
 * @param 		: 无
 * @return 		: 无
 */
static int __init mytimer_init(void)
{
	/* 初始化自旋锁 */
	spin_lock_init(&mytimerdev.lock);

	/* 注册字符设备驱动 */
	/* 1、创建设备号 */
	if (mytimerdev.major) {		/*  定义了设备号 */
		mytimerdev.devid = MKDEV(mytimerdev.major, 0);
		register_chrdev_region(mytimerdev.devid, MYTIMER_CNT, MYTIMER_NAME);
	} else {						/* 没有定义设备号 */
		alloc_chrdev_region(&mytimerdev.devid, 0, MYTIMER_CNT, MYTIMER_NAME);	/* 申请设备号 */
		mytimerdev.major = MAJOR(mytimerdev.devid);	/* 获取分配号的主设备号 */
		mytimerdev.minor = MINOR(mytimerdev.devid);	/* 获取分配号的次设备号 */
	}
	
	/* 2、初始化cdev */
	mytimerdev.cdev.owner = THIS_MODULE;
	cdev_init(&mytimerdev.cdev, &mytimer_fops);
	
	/* 3、添加一个cdev */
	cdev_add(&mytimerdev.cdev, mytimerdev.devid, MYTIMER_CNT);

	/* 4、创建类 */
	mytimerdev.class = class_create(THIS_MODULE, MYTIMER_NAME);
	if (IS_ERR(mytimerdev.class)) {
		return PTR_ERR(mytimerdev.class);
	}

	/* 5、创建设备 */
	mytimerdev.device = device_create(mytimerdev.class, NULL, mytimerdev.devid, NULL, MYTIMER_NAME);
	if (IS_ERR(mytimerdev.device)) {
		return PTR_ERR(mytimerdev.device);
	}
	
	/* 6、初始化timer，设置定时器处理函数,还未设置周期，所有不会激活定时器 */
	init_timer(&mytimerdev.timer);
	mytimerdev.timer.function = timer_function;
	mytimerdev.timer.data = (unsigned long)&mytimerdev;
	return 0;
}

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static void __exit mytimer_exit(void)
{
	
	gpio_set_value(mytimerdev.led_gpio, 1);	/* 卸载驱动的时候关闭LED */
	del_timer_sync(&mytimerdev.timer);		/* 删除timer */
#if 0
	del_timer(&mytimerdev.tiemr);
#endif

	/* 注销字符设备驱动 */
	gpio_free(mytimerdev.led_gpio);		
	cdev_del(&mytimerdev.cdev);/*  删除cdev */
	unregister_chrdev_region(mytimerdev.devid, MYTIMER_CNT); /* 注销设备号 */

	device_destroy(mytimerdev.class, mytimerdev.devid);
	class_destroy(mytimerdev.class);
}

module_init(mytimer_init);
module_exit(mytimer_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("rodney");
