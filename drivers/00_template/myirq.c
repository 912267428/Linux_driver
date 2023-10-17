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




#define MYIRQ_CNT		1			/* 设备号个数 	*/
#define MYIRQ_NAME		"myirq"	/* 名字 		*/
#define KEY0VALUE			0X01		/* KEY0按键值 	*/
#define INVAKEY				0XFF		/* 无效的按键值 */
#define KEY_NUM				1			/* 按键数量 	*/

/* 中断IO描述结构体 */
struct irq_keydesc {
	int gpio;								/* gpio */
	int irqnum;								/* 中断号     */
	unsigned char value;					/* 按键对应的键值 */
	char name[10];							/* 名字 */
	irqreturn_t (*handler)(int, void *);	/* 中断服务函数 */
};

/* myirq设备结构体 */
struct myirq_dev{
	dev_t devid;			/* 设备号 	 */
	struct cdev cdev;		/* cdev 	*/
	struct class *class;	/* 类 		*/
	struct device *device;	/* 设备 	 */
	int major;				/* 主设备号	  */
	int minor;				/* 次设备号   */
	struct device_node	*nd; /* 设备节点 */
	atomic_t keyvalue;		/* 有效的按键键值 */
	atomic_t releasekey;	/* 标记是否完成一次完成的按键，包括按下和释放 */
	struct timer_list timer;/* 定义一个定时器*/
	struct irq_keydesc irqkeydesc[KEY_NUM];	/* 按键描述数组 */
	unsigned char curkeynum;				/* 当前的按键号 */
};

struct myirq_dev myirq;	/* irq设备 */

/* @description		: 中断服务函数，开启定时器，延时10ms，
 *				  	  定时器用于按键消抖。
 * @param - irq 	: 中断号 
 * @param - dev_id	: 设备结构。
 * @return 			: 中断执行结果
 */
static irqreturn_t key0_handler(int irq, void *dev_id)
{
	struct myirq_dev *dev = (struct myirq_dev *)dev_id;

	dev->curkeynum = 0;
	dev->timer.data = (volatile long)dev_id;
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));	/* 10ms定时 */
	return IRQ_RETVAL(IRQ_HANDLED);
}

/* @description	: 定时器服务函数，用于按键消抖，定时器到了以后
 *				  再次读取按键值，如果按键还是处于按下状态就表示按键有效。
 * @param - arg	: 设备结构变量
 * @return 		: 无
 */
void timer_function(unsigned long arg)
{
	unsigned char value;
	unsigned char num;
	struct irq_keydesc *keydesc;
	struct myirq_dev *dev = (struct myirq_dev *)arg;

	num = dev->curkeynum;
	keydesc = &dev->irqkeydesc[num];

	value = gpio_get_value(keydesc->gpio); 	/* 读取IO值 */
	if(value == 0){ 						/* 按下按键 */
		atomic_set(&dev->keyvalue, keydesc->value);
	}
	else{ 									/* 按键松开 */
		atomic_set(&dev->keyvalue, 0x80 | keydesc->value);
		atomic_set(&dev->releasekey, 1);	/* 标记松开按键，即完成一次完整的按键过程 */			
	}	
}

/*
 * @description	: 按键IO初始化
 * @param 		: 无
 * @return 		: 无
 */
static int keyio_init(void)
{
	unsigned char i = 0;
	int ret = 0;
	
	myirq.nd = of_find_node_by_path("/key");
	if (myirq.nd== NULL){
		printk("key node not find!\r\n");
		return -EINVAL;
	} 

	/* 提取GPIO */
	for (i = 0; i < KEY_NUM; i++) {
		myirq.irqkeydesc[i].gpio = of_get_named_gpio(myirq.nd ,"key-gpio", i);
		if (myirq.irqkeydesc[i].gpio < 0) {
			printk("can't get key%d\r\n", i);
		}
	}
	
	/* 初始化key所使用的IO，并且设置成中断模式 */
	for (i = 0; i < KEY_NUM; i++) {
		memset(myirq.irqkeydesc[i].name, 0, sizeof(myirq.irqkeydesc[i].name));	/* 缓冲区清零 */
		sprintf(myirq.irqkeydesc[i].name, "KEY%d", i);		/* 组合名字 */
		gpio_request(myirq.irqkeydesc[i].gpio, myirq.irqkeydesc[i].name);
		gpio_direction_input(myirq.irqkeydesc[i].gpio);	
		myirq.irqkeydesc[i].irqnum = irq_of_parse_and_map(myirq.nd, i);
#if 0
		myirq.irqkeydesc[i].irqnum = gpio_to_irq(myirq.irqkeydesc[i].gpio);
#endif
		printk("key%d:gpio=%d, irqnum=%d\r\n",i, myirq.irqkeydesc[i].gpio, 
                                         myirq.irqkeydesc[i].irqnum);
	}
	/* 申请中断 */
	myirq.irqkeydesc[0].handler = key0_handler;
	myirq.irqkeydesc[0].value = KEY0VALUE;
	
	for (i = 0; i < KEY_NUM; i++) {
		ret = request_irq(myirq.irqkeydesc[i].irqnum, myirq.irqkeydesc[i].handler, 
		                 IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, myirq.irqkeydesc[i].name, &myirq);
		if(ret < 0){
			printk("irq %d request failed!\r\n", myirq.irqkeydesc[i].irqnum);
			return -EFAULT;
		}
	}

	/* 创建定时器 */
	init_timer(&myirq.timer);
	myirq.timer.function = timer_function;
	return 0;
}

/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int myirq_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &myirq;	/* 设置私有数据 */
	return 0;
}

 /*
  * @description     : 从设备读取数据 
  * @param - filp    : 要打开的设备文件(文件描述符)
  * @param - buf     : 返回给用户空间的数据缓冲区
  * @param - cnt     : 要读取的数据长度
  * @param - offt    : 相对于文件首地址的偏移
  * @return          : 读取的字节数，如果为负值，表示读取失败
  */
static ssize_t myirq_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	int ret = 0;
	unsigned char keyvalue = 0;
	unsigned char releasekey = 0;
	struct myirq_dev *dev = (struct myirq_dev *)filp->private_data;

	keyvalue = atomic_read(&dev->keyvalue);
	releasekey = atomic_read(&dev->releasekey);

	if (releasekey) { /* 有按键按下 */	
		if (keyvalue & 0x80) {
			keyvalue &= ~0x80;
			ret = copy_to_user(buf, &keyvalue, sizeof(keyvalue));
		} else {
			goto data_error;
		}
		atomic_set(&dev->releasekey, 0);/* 按下标志清零 */
	} else {
		goto data_error;
	}
	return 0;
	
data_error:
	return -EINVAL;
}

/* 设备操作函数 */
static struct file_operations myirq_fops = {
	.owner = THIS_MODULE,
	.open = myirq_open,
	.read = myirq_read,
};

/*
 * @description	: 驱动入口函数
 * @param 		: 无
 * @return 		: 无
 */
static int __init myirq_init(void)
{
	/* 1、构建设备号 */
	if (myirq.major) {
		myirq.devid = MKDEV(myirq.major, 0);
		register_chrdev_region(myirq.devid, MYIRQ_CNT, MYIRQ_NAME);
	} else {
		alloc_chrdev_region(&myirq.devid, 0, MYIRQ_CNT, MYIRQ_NAME);
		myirq.major = MAJOR(myirq.devid);
		myirq.minor = MINOR(myirq.devid);
	}

	/* 2、注册字符设备 */
	cdev_init(&myirq.cdev, &myirq_fops);
	cdev_add(&myirq.cdev, myirq.devid, MYIRQ_CNT);

	/* 3、创建类 */
	myirq.class = class_create(THIS_MODULE, MYIRQ_NAME);
	if (IS_ERR(myirq.class)) {
		return PTR_ERR(myirq.class);
	}

	/* 4、创建设备 */
	myirq.device = device_create(myirq.class, NULL, myirq.devid, NULL, MYIRQ_NAME);
	if (IS_ERR(myirq.device)) {
		return PTR_ERR(myirq.device);
	}
	
	/* 5、初始化按键 */
	atomic_set(&myirq.keyvalue, INVAKEY);
	atomic_set(&myirq.releasekey, 0);
	keyio_init();
	return 0;
}

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static void __exit myirq_exit(void)
{
	unsigned int i = 0;
	/* 删除定时器 */
	del_timer_sync(&myirq.timer);	/* 删除定时器 */
		
	/* 释放中断 */
	for (i = 0; i < KEY_NUM; i++) {
		free_irq(myirq.irqkeydesc[i].irqnum, &myirq);
		gpio_free(myirq.irqkeydesc[i].gpio);
	}
	cdev_del(&myirq.cdev);
	unregister_chrdev_region(myirq.devid, MYIRQ_CNT);
	device_destroy(myirq.class, myirq.devid);
	class_destroy(myirq.class);
}

module_init(myirq_init);
module_exit(myirq_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("rodney");
