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

#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define IRQ_CNT     1
#define IRQ_NAME    "noblockio"
#define KEY0VALUE   0x01
#define INVAKEY     0xFF
#define KEY_NUM     1

//中断IO描述结构体
struct noblockio_keydesc{
    int gpio;
    int irqnum; //中断号
    unsigned char value; //按键对应的键值
    char name[10];
    irqreturn_t (*handler)(int, void *);    //中断服务函数
};

struct noblockio_dev{
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
    struct noblockio_keydesc noblockiokeydesc[KEY_NUM];
    unsigned char curkeynum;

    wait_queue_head_t r_wait;   //读等待队列头
};

struct noblockio_dev noblockio;

//中断服务函数
static irqreturn_t key0_handler(int noblockio_num, void *dev_id)
{
    struct noblockio_dev *dev = (struct noblockio_dev *)dev_id;

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
    struct noblockio_keydesc *keydesc;
    struct noblockio_dev *dev = (struct noblockio_dev *)arg;

    num = dev->curkeynum;
    keydesc = &dev->noblockiokeydesc[num];

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
    noblockio.nd = of_find_node_by_path("/key");
    if (noblockio.nd == NULL)
    {
        printk("key node not find\r\n");
        return -EINVAL;
    }

    //获取gpio号和中断号
    for ( i = 0; i < KEY_NUM; i++)
    {
            noblockio.noblockiokeydesc[i].gpio = of_get_named_gpio(noblockio.nd, "key-gpio", i);
            if (noblockio.noblockiokeydesc[i].gpio < 0)
            {
                printk("can't get key%d\r\n", i);
            }
            

            for ( i = 0; i < KEY_NUM; i++)
            {
                memset(noblockio.noblockiokeydesc[i].name, 0, sizeof(noblockio.noblockiokeydesc[i].name));
                sprintf(noblockio.noblockiokeydesc[i].name, "KEY%d", i);
                gpio_request(noblockio.noblockiokeydesc[i].gpio, noblockio.noblockiokeydesc[i].name);
                gpio_direction_input(noblockio.noblockiokeydesc[i].gpio);
                noblockio.noblockiokeydesc[i].irqnum = irq_of_parse_and_map(noblockio.nd, i);//获取中断号
#if 0
                noblockio.noblockiokeydesc[i].irqnum = gpio_to_noblockio(noblockio.noblockiokeydesc[i].gpio);
#endif
                printk("key%d:gpio=%d, irqnum=%d\r\n", i, noblockio.noblockiokeydesc[i].gpio,noblockio.noblockiokeydesc[i].irqnum);
            }
            
    }

    //申请中断
    noblockio.noblockiokeydesc[0].handler = key0_handler;
    noblockio.noblockiokeydesc[0].value = KEY0VALUE;

    for ( i = 0; i < KEY_NUM; i++)
    {
        ret = request_irq(noblockio.noblockiokeydesc[i].irqnum,
                          noblockio.noblockiokeydesc[i].handler,
                          IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING,
                          noblockio.noblockiokeydesc[i].name, &noblockio);
        if (ret < 0)
        {
            printk("noblockio %d request failed\r\n",noblockio.noblockiokeydesc[i].irqnum);
            return -EFAULT;
        }
        
    }
    
    //创建定时器
    init_timer(&noblockio.timer);
    noblockio.timer.function = timer_function;

    //初始化等待队列头
    init_waitqueue_head(&noblockio.r_wait);

    return 0;
    
}

static int noblockio_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &noblockio;	/* 设置私有数据 */
	return 0;
}

static ssize_t noblockio_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	int ret = 0;
	unsigned char keyvalue = 0;
	unsigned char releasekey = 0;
	struct noblockio_dev *dev = (struct noblockio_dev *)filp->private_data;

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

unsigned int noblickio_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int mask = 0;
    struct noblockio_dev *dev = (struct noblockio_dev *)filp->private_data;

    poll_wait(filp, &dev->r_wait, wait);

    if (atomic_read(&dev->releasekey))
    {
        mask = POLLIN | POLLRDNORM;
    }

    return mask;
}

static struct file_operations noblockio_fops = {
	.owner = THIS_MODULE,
	.open = noblockio_open,
	.read = noblockio_read,
    .poll = noblickio_poll,
};

static int __init noblockio_init(void)
{
	/* 1、构建设备号 */
	if (noblockio.major) {
		noblockio.devid = MKDEV(noblockio.major, 0);
		register_chrdev_region(noblockio.devid, IRQ_CNT, IRQ_NAME);
	} else {
		alloc_chrdev_region(&noblockio.devid, 0, IRQ_CNT, IRQ_NAME);
		noblockio.major = MAJOR(noblockio.devid);
		noblockio.minor = MINOR(noblockio.devid);
	}

	/* 2、注册字符设备 */
	cdev_init(&noblockio.cdev, &noblockio_fops);
	cdev_add(&noblockio.cdev, noblockio.devid, IRQ_CNT);

	/* 3、创建类 */
	noblockio.class = class_create(THIS_MODULE, IRQ_NAME);
	if (IS_ERR(noblockio.class)) {
		return PTR_ERR(noblockio.class);
	}

	/* 4、创建设备 */
	noblockio.device = device_create(noblockio.class, NULL, noblockio.devid, NULL, IRQ_NAME);
	if (IS_ERR(noblockio.device)) {
		return PTR_ERR(noblockio.device);
	}
	
	/* 5、初始化按键 */
	atomic_set(&noblockio.keyvalue, INVAKEY);
	atomic_set(&noblockio.releasekey, 0);
	keyio_init();
	return 0;
}



static void __exit anoblockio_exit(void)
{
	unsigned int i = 0;
	/* 删除定时器 */
	del_timer_sync(&noblockio.timer);	/* 删除定时器 */
		
	/* 释放中断 */
	for (i = 0; i < KEY_NUM; i++) {
		free_irq(noblockio.noblockiokeydesc[i].irqnum, &noblockio);
		gpio_free(noblockio.noblockiokeydesc[i].gpio);
	}
	cdev_del(&noblockio.cdev);
	unregister_chrdev_region(noblockio.devid, IRQ_CNT);
	device_destroy(noblockio.class, noblockio.devid);
	class_destroy(noblockio.class);
}

module_init(noblockio_init);
module_exit(anoblockio_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("rodney");