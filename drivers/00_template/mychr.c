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
 * 使用的名字mychr
 * 用需要使用的名字替换"mychr" 大小写均要
 * 
 * 
*/

#define DIR_IN

#define MYCHR_CNT		1		
#define MYCHR_NAME		"mychr"	
#define LEDOFF 			0			/* 关灯 */
#define LEDON 			1			/* 开灯 */

struct mychr_dev
{
    dev_t devid;			/* 设备号 	 */
	struct cdev cdev;		/* cdev 	*/
	struct class *class;	/* 类 		*/
	struct device *device;	/* 设备 	 */
	int major;				/* 主设备号	  */
	int minor;				/* 次设备号   */
	struct device_node	*nd; /* 设备节点 */
	int mychr_gpio;			/* led所使用的GPIO编号		*/
	struct mutex lock;		/* 互斥体  使用互斥体以保证只有一个程序可以使用驱动*/
};

struct mychr_dev mychrdev;

/*
 * @brief	      初始化按键IO，open函数打开驱动的时候
 * 				  初始化按键所使用的GPIO引脚。
 * @param 		  无
 * @retval 	      无
 */
static int mychrio_init(unsigned char is_in)
{
	int ret = 0;

    if (is_in)
        mychrdev.nd = of_find_node_by_path("/key");
    else
        mychrdev.nd = of_find_node_by_path("/gpioled");
	if (mychrdev.nd== NULL) {
		return -EINVAL;
	}else{
		printk("chr-gpio node find!\r\n");
	}

    if (is_in)
	    mychrdev.mychr_gpio = of_get_named_gpio(mychrdev.nd ,"key-gpio", 0);
    else
        mychrdev.mychr_gpio = of_get_named_gpio(mychrdev.nd ,"led-gpio", 0);

	if (mychrdev.mychr_gpio < 0) {
		printk("can't get gpio\r\n");
		return -EINVAL;
	}
	printk("mychr_gpio=%d\r\n", mychrdev.mychr_gpio);
	
	/* 初始化key所使用的IO */
	gpio_request(mychrdev.mychr_gpio, "mychr");	/* 请求IO */

    if (is_in)
	    ret = gpio_direction_input(mychrdev.mychr_gpio);	/* 设置为输入 */
    else
        ret = gpio_direction_output(mychrdev.mychr_gpio, 1);
	if (ret < 0)
	{
		printk("can't set gpio!\r\n");
		return -1;
	}
	
	return 0;
}

/*
 * @brief		    打开设备
 * @param inode     传递给驱动的inode
 * @param filp 	    设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @retval  0 成功;其他 失败
 */
static int mychr_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &mychrdev;

    /* 获取互斥体,可以被信号打断 */
	if (mutex_lock_interruptible(&mychrdev.lock)) {
		return -ERESTARTSYS;
	}
#if 0
	mutex_lock(&mychrdev.lock);	/* 不能被信号打断 */
#endif

	return 0;
}

/*
 * @brief		 关闭/释放设备
 * @param filp 	 要关闭的设备文件(文件描述符)
 * @retval 		 0 成功;其他 失败
 */
static int mychr_release(struct inode *inode, struct file *filp)
{
	struct mychr_dev *dev = filp->private_data;

	/* 释放互斥锁 */
	mutex_unlock(&dev->lock);

	return 0;
}

/*
 * @brief		    从设备读取数据 
 * @param filp      设备文件，表示打开的文件描述符
 * @param buf 	    返回给用户空间的数据缓冲区
 * @param cnt       要写入的数据长度
 * @param offt      相对于文件首地址的偏移
 * @retval  0 成功;其他 失败
 */
static ssize_t mychr_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	int ret = 0;
	int value;
	struct mychr_dev *dev = filp->private_data;

	if (gpio_get_value(dev->mychr_gpio) == 0){
		while(!gpio_get_value(dev->mychr_gpio));	/* 等待按键释放 */
		value = 0;
	}
	else{
		value = 1;
	}
	ret = copy_to_user(buf, &value, sizeof(value));
	return 0;
}

/*
 * @brief		    向设备写数据 
 * @param filp      设备文件，表示打开的文件描述符
 * @param buf 	    要写给设备写入的数据
 * @param cnt       要写入的数据长度
 * @param offt      相对于文件首地址的偏移
 * @retval  0 成功;其他 失败
 */
static ssize_t mychr_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue;
	unsigned char databuf[1];
	unsigned char ledstat;
	struct mychr_dev *dev = filp->private_data;

	retvalue = copy_from_user(databuf, buf, cnt);
	if(retvalue < 0) {
		printk("kernel write failed!\r\n");
		return -EFAULT;
	}

	ledstat = databuf[0];		/* 获取状态值 */

	if(ledstat == LEDON) {	
		gpio_set_value(dev->mychr_gpio, 0);	/* 打开LED灯 */
	} else if(ledstat == LEDOFF) {
		gpio_set_value(dev->mychr_gpio, 1);	/* 关闭LED灯 */
	}
	return 0;
}

static struct file_operations mychr_fops = {
	.owner = THIS_MODULE,
	.open = mychr_open,
	.read = mychr_read,
	.write = mychr_write,
	.release = 	mychr_release,
};

/*
 * @brief	驱动入口函数
 * @param 	无
 * @retval 	无
 */
static int __init mychr_init(void)
{
	int ret = 0;
    /* 初始化互斥体 */
	mutex_init(&mychrdev.lock);

    /* 注册字符设备驱动 */
	/* 1、创建设备号 */
	if (mychrdev.major) {		/*  定义了设备号 */
		mychrdev.devid = MKDEV(mychrdev.major, 0);
		register_chrdev_region(mychrdev.devid, MYCHR_CNT, MYCHR_NAME);
	} else {						/* 没有定义设备号 */
		alloc_chrdev_region(&mychrdev.devid, 0, MYCHR_CNT, MYCHR_NAME);	/* 申请设备号 */
		mychrdev.major = MAJOR(mychrdev.devid);	/* 获取分配号的主设备号 */
		mychrdev.minor = MINOR(mychrdev.devid);	/* 获取分配号的次设备号 */
	}
	printk("mychr major=%d,minor=%d\r\n",mychrdev.major, mychrdev.minor);	
	
	/* 2、初始化cdev */
	mychrdev.cdev.owner = THIS_MODULE;
	cdev_init(&mychrdev.cdev, &mychr_fops);
	
	/* 3、添加一个cdev */
	cdev_add(&mychrdev.cdev, mychrdev.devid, MYCHR_CNT);

	/* 4、创建类 */
	mychrdev.class = class_create(THIS_MODULE, MYCHR_NAME);
	if (IS_ERR(mychrdev.class)) {
		return PTR_ERR(mychrdev.class);
	}

	/* 5、创建设备 */
	mychrdev.device = device_create(mychrdev.class, NULL, mychrdev.devid, NULL, MYCHR_NAME);
	if (IS_ERR(mychrdev.device)) {
		return PTR_ERR(mychrdev.device);
	}

    //初始化IO
#ifdef DIR_IN
    ret = mychrio_init(1);
#else
    ret = mychrio_init(0);
#endif
	if (ret < 0)
	{
		printk("IO init fail\r\n");
		return ret;
	}
	return 0;
}

/*
 * @brief	驱动出口函数
 * @param 	无
 * @retval 	无
 */
static void __exit mychr_exit(void)
{
    /* 注销字符设备驱动 */
	gpio_free(mychrdev.mychr_gpio);
	cdev_del(&mychrdev.cdev);/*  删除cdev */
	unregister_chrdev_region(mychrdev.devid, MYCHR_CNT); /* 注销设备号 */

	device_destroy(mychrdev.class, mychrdev.devid);
	class_destroy(mychrdev.class);
}


module_init(mychr_init);
module_exit(mychr_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("rodney");