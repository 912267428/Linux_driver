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

#define IRQ_CNT     1
#define IRQ_NAME    "blockio"
#define KEY0VALUE   0x01
#define INVAKEY     0xFF
#define KEY_NUM     1

//中断IO描述结构体
struct blockio_keydesc{
    int gpio;
    int irqnum; //中断号
    unsigned char value; //按键对应的键值
    char name[10];
    irqreturn_t (*handler)(int, void *);    //中断服务函数
};

struct blockio_dev{
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
    struct blockio_keydesc blockiokeydesc[KEY_NUM];
    unsigned char curkeynum;

    wait_queue_head_t r_wait;   //读等待队列头
};

struct blockio_dev blockio;

//中断服务函数
static irqreturn_t key0_handler(int blockio_num, void *dev_id)
{
    struct blockio_dev *dev = (struct blockio_dev *)dev_id;

    dev->curkeynum = 0;
    dev->timer.data = (volatile long)dev_id;
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));
    return IRQ_RETVAL(IRQ_HANDLED);
}

//定时器服务函数，用于按键消抖
void timer_function(unsigned long arg)
{
    unsigned char value;
    unsigned char num;
    struct blockio_keydesc *keydesc;
    struct blockio_dev *dev = (struct blockio_dev *)arg;

    num = dev->curkeynum;
    keydesc = &dev->blockiokeydesc[num];

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

    //定时器服务函数完成，表示消抖已完成，所以唤醒进程
    if (atomic_read(&dev->releasekey))
    {
        wake_up_interruptible(&dev->r_wait);
    }
    
}

//按键IO初始化
static int keyio_init(void)
{
    unsigned char i = 0;
    int ret = 0;

    //找到设备树节点
    blockio.nd = of_find_node_by_path("/key");
    if (blockio.nd == NULL)
    {
        printk("key node not find\r\n");
        return -EINVAL;
    }

    //获取gpio号和中断号
    for ( i = 0; i < KEY_NUM; i++)
    {
            blockio.blockiokeydesc[i].gpio = of_get_named_gpio(blockio.nd, "key-gpio", i);
            if (blockio.blockiokeydesc[i].gpio < 0)
            {
                printk("can't get key%d\r\n", i);
            }
            

            for ( i = 0; i < KEY_NUM; i++)
            {
                memset(blockio.blockiokeydesc[i].name, 0, sizeof(blockio.blockiokeydesc[i].name));
                sprintf(blockio.blockiokeydesc[i].name, "KEY%d", i);
                gpio_request(blockio.blockiokeydesc[i].gpio, blockio.blockiokeydesc[i].name);
                gpio_direction_input(blockio.blockiokeydesc[i].gpio);
                blockio.blockiokeydesc[i].irqnum = irq_of_parse_and_map(blockio.nd, i);//获取中断号
#if 0
                blockio.blockiokeydesc[i].irqnum = gpio_to_blockio(blockio.blockiokeydesc[i].gpio);
#endif
                printk("key%d:gpio=%d, irqnum=%d\r\n", i, blockio.blockiokeydesc[i].gpio,blockio.blockiokeydesc[i].irqnum);
            }
            
    }

    //申请中断
    blockio.blockiokeydesc[0].handler = key0_handler;
    blockio.blockiokeydesc[0].value = KEY0VALUE;

    for ( i = 0; i < KEY_NUM; i++)
    {
        ret = request_irq(blockio.blockiokeydesc[i].irqnum,
                          blockio.blockiokeydesc[i].handler,
                          IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING,
                          blockio.blockiokeydesc[i].name, &blockio);
        if (ret < 0)
        {
            printk("blockio %d request failed\r\n",blockio.blockiokeydesc[i].irqnum);
            return -EFAULT;
        }
        
    }
    
    //创建定时器
    init_timer(&blockio.timer);
    blockio.timer.function = timer_function;

    //初始化等待队列头
    init_waitqueue_head(&blockio.r_wait);

    return 0;
    
}

static int blockio_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &blockio;	/* 设置私有数据 */
	return 0;
}

static ssize_t blockio_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	int ret = 0;
	unsigned char keyvalue = 0;
	unsigned char releasekey = 0;
	struct blockio_dev *dev = (struct blockio_dev *)filp->private_data;

#if 0
    ret = wait_event_interruptible(dev->r_wait, atomic_read(&dev->releasekey));
    if (ret)
    {
        goto wait_error;
    }
#endif 
    DECLARE_WAITQUEUE(wait, current);
    if (atomic_read(&dev->releasekey) == 0) //按键没有按下
    {
        add_wait_queue(&dev->r_wait, &wait);
        __set_current_state(TASK_INTERRUPTIBLE);
        schedule();
        if (signal_pending(current)) //判断是否是信号引起的唤醒
        {
            ret = -ERESTARTSYS;
            goto wait_error;
        }

        __set_current_state(TASK_RUNNING);
        remove_wait_queue(&dev->r_wait, &wait);
        
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
    set_current_state(TASK_RUNNING);
    remove_wait_queue(&dev->r_wait, &wait);

data_error:
	return -EINVAL;
}

static struct file_operations blockio_fops = {
	.owner = THIS_MODULE,
	.open = blockio_open,
	.read = blockio_read,
};

static int __init blockio_init(void)
{
	/* 1、构建设备号 */
	if (blockio.major) {
		blockio.devid = MKDEV(blockio.major, 0);
		register_chrdev_region(blockio.devid, IRQ_CNT, IRQ_NAME);
	} else {
		alloc_chrdev_region(&blockio.devid, 0, IRQ_CNT, IRQ_NAME);
		blockio.major = MAJOR(blockio.devid);
		blockio.minor = MINOR(blockio.devid);
	}

	/* 2、注册字符设备 */
	cdev_init(&blockio.cdev, &blockio_fops);
	cdev_add(&blockio.cdev, blockio.devid, IRQ_CNT);

	/* 3、创建类 */
	blockio.class = class_create(THIS_MODULE, IRQ_NAME);
	if (IS_ERR(blockio.class)) {
		return PTR_ERR(blockio.class);
	}

	/* 4、创建设备 */
	blockio.device = device_create(blockio.class, NULL, blockio.devid, NULL, IRQ_NAME);
	if (IS_ERR(blockio.device)) {
		return PTR_ERR(blockio.device);
	}
	
	/* 5、初始化按键 */
	atomic_set(&blockio.keyvalue, INVAKEY);
	atomic_set(&blockio.releasekey, 0);
	keyio_init();
	return 0;
}



static void __exit ablockio_exit(void)
{
	unsigned int i = 0;
	/* 删除定时器 */
	del_timer_sync(&blockio.timer);	/* 删除定时器 */
		
	/* 释放中断 */
	for (i = 0; i < KEY_NUM; i++) {
		free_irq(blockio.blockiokeydesc[i].irqnum, &blockio);
		gpio_free(blockio.blockiokeydesc[i].gpio);
	}
	cdev_del(&blockio.cdev);
	unregister_chrdev_region(blockio.devid, IRQ_CNT);
	device_destroy(blockio.class, blockio.devid);
	class_destroy(blockio.class);
}

module_init(blockio_init);
module_exit(ablockio_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("rodney");