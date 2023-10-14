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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define TIMER_CNT       1
#define TIMER_NAME      "timer"
#define CLOSE_CMD       (_IO(0xEF, 0x1))
#define OPEN_CMD        (_IO(0xEF, 0x2))
#define SETPERIOD_CMD   (_IO(0xEF, 0x3))    //设置定时器周期命令
#define LEDON           1
#define LEDOFF          0

struct timer_dev{
	dev_t	devid;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	int major;
	int minor;
	struct device_node *nd;
	int led_gpio;
	
    int timeperiod;
    struct timer_list timer;
    spinlock_t lock;
};

struct timer_dev timerdev;

static int led_init(void)
{
    int ret = 0;

    timerdev.nd = of_find_node_by_path("/gpioled");
    if (timerdev.nd == NULL)
    {
        return -EINVAL;
    }

    timerdev.led_gpio = of_get_named_gpio(timerdev.nd, "led-gpio", 0);
    if (timerdev.led_gpio < 0)
    {
        printk("can't get led\r\n");
        return -EINVAL;
    }

    gpio_request(timerdev.led_gpio, "led");
    ret = gpio_direction_output(timerdev.led_gpio, 1);
    if (ret < 0)
    {
        printk("can't set gpio\r\n");
    }

    return 0;
}

static int timer_open(struct inode *inode, struct file *filp)
{
    int ret = 0;
    filp->private_data = &timerdev;

    timerdev.timeperiod = 1000; //默认周期为1s
    ret = led_init();
    if (ret < 0)
    {
        return ret;
    }
    
    return 0;
}


//ioctl函数，处理应用程序发送过来的命令
static long timer_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct timer_dev *dev = (struct timer_dev *)filp->private_data;
    int timerperiod;
    unsigned long flags;

    switch (cmd)
    {
    case CLOSE_CMD:
        del_timer_sync(&dev->timer);
        break;
    case OPEN_CMD:
        spin_lock_irqsave(&dev->lock, flags);
        timerperiod = dev->timeperiod;
        spin_unlock_irqrestore(&dev->lock, flags);
        mod_timer(&dev->timer, jiffies+msecs_to_jiffies(timerperiod));
        break;
    case SETPERIOD_CMD:
        spin_lock_irqsave(&dev->lock, flags);
        dev->timeperiod = arg;
        spin_unlock_irqrestore(&dev->lock, flags);
        mod_timer(&dev->timer, jiffies+msecs_to_jiffies(arg));
    default:
        break;
    }

    return 0;
}

static struct file_operations timer_fops = {
    .owner = THIS_MODULE,
    .open = timer_open,
    .unlocked_ioctl = timer_unlocked_ioctl,
};

void timer_function(unsigned long arg)
{
    struct timer_dev *dev = (struct timer_dev*)arg;
    static int sta = 1;
    int timerperiod;
    unsigned long flags;

    sta = !sta;
    gpio_set_value(dev->led_gpio, sta);

    spin_lock_irqsave(&dev->lock, flags);
    timerperiod = dev->timeperiod;
    spin_unlock_irqrestore(&dev->lock, flags);
    mod_timer(&dev->timer, jiffies+msecs_to_jiffies(dev->timeperiod));
}

static int __init timer_init(void)
{
    spin_lock_init(&timerdev.lock);

    /* 注册字符设备驱动 */
	/* 1、创建设备号 */
	if (timerdev.major) {		/*  定义了设备号 */
		timerdev.devid = MKDEV(timerdev.major, 0);
		register_chrdev_region(timerdev.devid, TIMER_CNT, TIMER_NAME);
	} else {						/* 没有定义设备号 */
		alloc_chrdev_region(&timerdev.devid, 0, TIMER_CNT, TIMER_NAME);	/* 申请设备号 */
		timerdev.major = MAJOR(timerdev.devid);	/* 获取分配号的主设备号 */
		timerdev.minor = MINOR(timerdev.devid);	/* 获取分配号的次设备号 */
	}
	
	/* 2、初始化cdev */
	timerdev.cdev.owner = THIS_MODULE;
	cdev_init(&timerdev.cdev, &timer_fops);
	
	/* 3、添加一个cdev */
	cdev_add(&timerdev.cdev, timerdev.devid, TIMER_CNT);

	/* 4、创建类 */
	timerdev.class = class_create(THIS_MODULE, TIMER_NAME);
	if (IS_ERR(timerdev.class)) {
		return PTR_ERR(timerdev.class);
	}

	/* 5、创建设备 */
	timerdev.device = device_create(timerdev.class, NULL, timerdev.devid, NULL, TIMER_NAME);
	if (IS_ERR(timerdev.device)) {
		return PTR_ERR(timerdev.device);
	}

    init_timer(&timerdev.timer);
    timerdev.timer.function = timer_function;
    timerdev.timer.data = (unsigned long)&timerdev;

    return 0;
}
static void __exit timer_exit(void)
{
    gpio_set_value(timerdev.led_gpio, 1);
    del_timer_sync(&timerdev.timer);
#if 0
	del_timer(&timerdev.tiemr);
#endif
    /* 注销字符设备驱动 */
	gpio_free(timerdev.led_gpio);		
	cdev_del(&timerdev.cdev);/*  删除cdev */
	unregister_chrdev_region(timerdev.devid, TIMER_CNT); /* 注销设备号 */

	device_destroy(timerdev.class, timerdev.devid);
	class_destroy(timerdev.class);
}


module_init(timer_init);
module_exit(timer_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("rodney");