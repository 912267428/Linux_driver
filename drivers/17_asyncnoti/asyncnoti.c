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
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fcntl.h>

#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define IRQ_CNT     1
#define IRQ_NAME    "asyncnoti"
#define KEY0VALUE   0x01
#define INVAKEY     0xFF
#define KEY_NUM     1

//中断IO描述结构体
struct asyncnoti_keydesc{
    int gpio;
    int irqnum; //中断号
    unsigned char value; //按键对应的键值
    char name[10];
    irqreturn_t (*handler)(int, void *);    //中断服务函数
};

struct asyncnoti_dev{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *nd;
    atomic_t keyvalue;
    atomic_t releasekey;    //标记是否完成一次的按键
    
    struct timer_list timer;
    struct asyncnoti_keydesc asyncnotikeydesc[KEY_NUM];
    unsigned char curkeynum;

    wait_queue_head_t r_wait;   //读等待队列头

    //异步相关结构体
    struct fasync_struct *async_ququ;
};

struct asyncnoti_dev asyncnoti;

//中断服务函数
static irqreturn_t key0_handler(int asyncnoti_num, void *dev_id)
{
    struct asyncnoti_dev *dev = (struct asyncnoti_dev *)dev_id;

    dev->curkeynum = 0;
    dev->timer.data = (volatile long)dev_id;
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(15));
    return IRQ_RETVAL(IRQ_HANDLED);
}

//定时器服务函数，用于按键消抖
void timer_function(unsigned long arg)
{
    unsigned char value;
    unsigned char num;
    struct asyncnoti_keydesc *keydesc;
    struct asyncnoti_dev *dev = (struct asyncnoti_dev *)arg;

    num = dev->curkeynum;
    keydesc = &dev->asyncnotikeydesc[num];

    value = gpio_get_value(keydesc->gpio);
    if (value == 0)
    {
        printk("Key Push!\r\n");
        atomic_set(&dev->keyvalue,keydesc->value);
    }else{
        printk("Key Release!\r\n");
        atomic_set(&dev->keyvalue, 0x80 | keydesc->value);
        atomic_set(&dev->releasekey,1);
    }

    //异步
    if (atomic_read(&dev->releasekey))  //完成一次按键过程
    {
        if (dev->async_ququ)
        {
            kill_fasync(&dev->async_ququ, SIGIO, POLL_IN); //发送信号
        }
    }

#if 0
    //定时器服务函数完成，表示消抖已完成，所以唤醒进程
    if (atomic_read(&dev->releasekey))
    {
        wake_up_interruptible(&dev->r_wait);
    }
#endif   
}

//按键IO初始化
static int keyio_init(void)
{
    unsigned char i = 0;
    int ret = 0;

    //找到设备树节点
    asyncnoti.nd = of_find_node_by_path("/key");
    if (asyncnoti.nd == NULL)
    {
        printk("key node not find\r\n");
        return -EINVAL;
    }

    //获取gpio号和中断号
    for ( i = 0; i < KEY_NUM; i++)
    {
            asyncnoti.asyncnotikeydesc[i].gpio = of_get_named_gpio(asyncnoti.nd, "key-gpio", i);
            if (asyncnoti.asyncnotikeydesc[i].gpio < 0)
            {
                printk("can't get key%d\r\n", i);
            }
            

            for ( i = 0; i < KEY_NUM; i++)
            {
                memset(asyncnoti.asyncnotikeydesc[i].name, 0, sizeof(asyncnoti.asyncnotikeydesc[i].name));
                sprintf(asyncnoti.asyncnotikeydesc[i].name, "KEY%d", i);
                gpio_request(asyncnoti.asyncnotikeydesc[i].gpio, asyncnoti.asyncnotikeydesc[i].name);
                gpio_direction_input(asyncnoti.asyncnotikeydesc[i].gpio);
                asyncnoti.asyncnotikeydesc[i].irqnum = irq_of_parse_and_map(asyncnoti.nd, i);//获取中断号
#if 0
                asyncnoti.asyncnotikeydesc[i].irqnum = gpio_to_asyncnoti(asyncnoti.asyncnotikeydesc[i].gpio);
#endif
                printk("key%d:gpio=%d, irqnum=%d\r\n", i, asyncnoti.asyncnotikeydesc[i].gpio,asyncnoti.asyncnotikeydesc[i].irqnum);
            }
            
    }

    //申请中断
    asyncnoti.asyncnotikeydesc[0].handler = key0_handler;
    asyncnoti.asyncnotikeydesc[0].value = KEY0VALUE;

    for ( i = 0; i < KEY_NUM; i++)
    {
        ret = request_irq(asyncnoti.asyncnotikeydesc[i].irqnum,
                          asyncnoti.asyncnotikeydesc[i].handler,
                          IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING,
                          asyncnoti.asyncnotikeydesc[i].name, &asyncnoti);
        if (ret < 0)
        {
            printk("asyncnoti %d request failed\r\n",asyncnoti.asyncnotikeydesc[i].irqnum);
            return -EFAULT;
        }
        
    }
    
    //创建定时器
    init_timer(&asyncnoti.timer);
    asyncnoti.timer.function = timer_function;

    //初始化等待队列头
    init_waitqueue_head(&asyncnoti.r_wait);

    return 0;
    
}

static int asyncnoti_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &asyncnoti;	/* 设置私有数据 */
	return 0;
}

static ssize_t asyncnoti_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	int ret = 0;
	unsigned char keyvalue = 0;
	unsigned char releasekey = 0;
	struct asyncnoti_dev *dev = (struct asyncnoti_dev *)filp->private_data;

    if (filp->f_flags & O_NONBLOCK)
    {
        if (atomic_read(&dev->releasekey) == 0)
        {
            return -EAGAIN;
        }else{
            // 加入等待队列，等待被唤醒
            ret = wait_event_interruptible(dev->r_wait, atomic_read(&dev->releasekey));
            if (ret)
            {
                goto wait_error;
            }
            
        }
        
    }
    
    

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

wait_error:
    return ret;

data_error:
	return -EINVAL;
}

static unsigned int asyncnoti_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int mask = 0;
    struct asyncnoti_dev *dev = (struct asyncnoti_dev *)filp->private_data;

    poll_wait(filp, &dev->r_wait, wait);

    if (atomic_read(&dev->releasekey))
    {
        mask = POLLIN | POLLRDNORM;
    }

    return mask;
}

static int  asyncnoti_fasync(int fd, struct file *filp, int on)
{
    struct asyncnoti_dev *dev = (struct asyncnoti_dev *)filp->private_data;

    return fasync_helper(fd, filp, on, &dev->async_ququ);
}

static int asyncnoti_release(struct inode *inode, struct file *filp)
{
    return asyncnoti_fasync(-1, filp, 0);
}

static struct file_operations asyncnoti_fops = {
	.owner = THIS_MODULE,
	.open = asyncnoti_open,
    .release = asyncnoti_release,
	.read = asyncnoti_read,
    .poll = asyncnoti_poll,
    .fasync = asyncnoti_fasync,
};

static int __init asyncnoti_init(void)
{
	/* 1、构建设备号 */
	if (asyncnoti.major) {
		asyncnoti.devid = MKDEV(asyncnoti.major, 0);
		register_chrdev_region(asyncnoti.devid, IRQ_CNT, IRQ_NAME);
	} else {
		alloc_chrdev_region(&asyncnoti.devid, 0, IRQ_CNT, IRQ_NAME);
		asyncnoti.major = MAJOR(asyncnoti.devid);
		asyncnoti.minor = MINOR(asyncnoti.devid);
	}

	/* 2、注册字符设备 */
	cdev_init(&asyncnoti.cdev, &asyncnoti_fops);
	cdev_add(&asyncnoti.cdev, asyncnoti.devid, IRQ_CNT);

	/* 3、创建类 */
	asyncnoti.class = class_create(THIS_MODULE, IRQ_NAME);
	if (IS_ERR(asyncnoti.class)) {
		return PTR_ERR(asyncnoti.class);
	}

	/* 4、创建设备 */
	asyncnoti.device = device_create(asyncnoti.class, NULL, asyncnoti.devid, NULL, IRQ_NAME);
	if (IS_ERR(asyncnoti.device)) {
		return PTR_ERR(asyncnoti.device);
	}
	
	/* 5、初始化按键 */
	atomic_set(&asyncnoti.keyvalue, INVAKEY);
	atomic_set(&asyncnoti.releasekey, 0);
	keyio_init();
	return 0;
}



static void __exit aasyncnoti_exit(void)
{
	unsigned int i = 0;
	/* 删除定时器 */
	del_timer_sync(&asyncnoti.timer);	/* 删除定时器 */
		
	/* 释放中断 */
	for (i = 0; i < KEY_NUM; i++) {
		free_irq(asyncnoti.asyncnotikeydesc[i].irqnum, &asyncnoti);
		gpio_free(asyncnoti.asyncnotikeydesc[i].gpio);
	}
	cdev_del(&asyncnoti.cdev);
	unregister_chrdev_region(asyncnoti.devid, IRQ_CNT);
	device_destroy(asyncnoti.class, asyncnoti.devid);
	class_destroy(asyncnoti.class);
}

module_init(asyncnoti_init);
module_exit(aasyncnoti_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("rodney");