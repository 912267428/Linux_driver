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
#define IRQ_NAME    "irq"
#define KEY0VALUE   0x01
#define INVAKEY     0xFF
#define KEY_NUM     1

//中断IO描述结构体
struct irq_keydesc{
    int gpio;
    int irqnum; //中断号
    unsigned char value; //按键对应的键值
    char name[10];
    irqreturn_t (*handler)(int, void *);    //中断服务函数
};

struct irq_dev{
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
    struct irq_keydesc irqkeydesc[KEY_NUM];
    unsigned char curkeynum;
};

struct irq_dev irq;

//中断服务函数
static irqreturn_t key0_handler(int irq_num, void *dev_id)
{
    struct irq_dev *dev = (struct irq_dev *)dev_id;

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
    struct irq_keydesc *keydesc;
    struct irq_dev *dev = (struct irq_dev *)arg;

    num = dev->curkeynum;
    keydesc = &dev->irqkeydesc[num];

    value = gpio_get_value(keydesc->gpio);
    if (value == 0)
    {
        atomic_set(&dev->keyvalue,keydesc->value);
    }else{
        atomic_set(&dev->keyvalue, 0x80 | keydesc->value);
        atomic_set(&dev->releasekey,1);
    }
    
}

//按键IO初始化
static int keyio_init(void)
{
    unsigned char i = 0;
    int ret = 0;

    //找到设备树节点
    irq.nd = of_find_node_by_path("/key");
    if (irq.nd == NULL)
    {
        printk("key node not find\r\n");
        return -EINVAL;
    }

    //获取gpio号和中断号
    for ( i = 0; i < KEY_NUM; i++)
    {
            irq.irqkeydesc[i].gpio = of_get_named_gpio(irq.nd, "key-gpio", i);
            if (irq.irqkeydesc[i].gpio < 0)
            {
                printk("can't get key%d\r\n", i);
            }
            

            for ( i = 0; i < KEY_NUM; i++)
            {
                memset(irq.irqkeydesc[i].name, 0, sizeof(irq.irqkeydesc[i].name));
                sprintf(irq.irqkeydesc[i].name, "KEY%d", i);
                gpio_request(irq.irqkeydesc[i].gpio, irq.irqkeydesc[i].name);
                gpio_direction_input(irq.irqkeydesc[i].gpio);
                irq.irqkeydesc[i].irqnum = irq_of_parse_and_map(irq.nd, i);//获取中断号
#if 0
                irq.irqkeydesc[i].irqnum = gpio_to_irq(irq.irqkeydesc[i].gpio);
#endif
                printk("key%d:gpio=%d, irqnum=%d\r\n", i, irq.irqkeydesc[i].gpio,irq.irqkeydesc[i].irqnum);
            }
            
    }

    //申请中断
    irq.irqkeydesc[0].handler = key0_handler;
    irq.irqkeydesc[0].value = KEY0VALUE;

    for ( i = 0; i < KEY_NUM; i++)
    {
        ret = request_irq(irq.irqkeydesc[i].irqnum,
                          irq.irqkeydesc[i].handler,
                          IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING,
                          irq.irqkeydesc[i].name, &irq);
        if (ret < 0)
        {
            printk("irq %d request failed\r\n",irq.irqkeydesc[i].irqnum);
            return -EFAULT;
        }
        
    }
    
    //创建定时器
    init_timer(&irq.timer);
    irq.timer.function = timer_function;
    return 0;
    
}

static int irq_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &irq;	/* 设置私有数据 */
	return 0;
}

static ssize_t irq_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	int ret = 0;
	unsigned char keyvalue = 0;
	unsigned char releasekey = 0;
	struct irq_dev *dev = (struct irq_dev *)filp->private_data;

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

static struct file_operations irq_fops = {
	.owner = THIS_MODULE,
	.open = irq_open,
	.read = irq_read,
};

static int __init irq_init(void)
{
	/* 1、构建设备号 */
	if (irq.major) {
		irq.devid = MKDEV(irq.major, 0);
		register_chrdev_region(irq.devid, IRQ_CNT, IRQ_NAME);
	} else {
		alloc_chrdev_region(&irq.devid, 0, IRQ_CNT, IRQ_NAME);
		irq.major = MAJOR(irq.devid);
		irq.minor = MINOR(irq.devid);
	}

	/* 2、注册字符设备 */
	cdev_init(&irq.cdev, &irq_fops);
	cdev_add(&irq.cdev, irq.devid, IRQ_CNT);

	/* 3、创建类 */
	irq.class = class_create(THIS_MODULE, IRQ_NAME);
	if (IS_ERR(irq.class)) {
		return PTR_ERR(irq.class);
	}

	/* 4、创建设备 */
	irq.device = device_create(irq.class, NULL, irq.devid, NULL, IRQ_NAME);
	if (IS_ERR(irq.device)) {
		return PTR_ERR(irq.device);
	}
	
	/* 5、初始化按键 */
	atomic_set(&irq.keyvalue, INVAKEY);
	atomic_set(&irq.releasekey, 0);
	keyio_init();
	return 0;
}



static void __exit airq_exit(void)
{
	unsigned int i = 0;
	/* 删除定时器 */
	del_timer_sync(&irq.timer);	/* 删除定时器 */
		
	/* 释放中断 */
	for (i = 0; i < KEY_NUM; i++) {
		free_irq(irq.irqkeydesc[i].irqnum, &irq);
		gpio_free(irq.irqkeydesc[i].gpio);
	}
	cdev_del(&irq.cdev);
	unregister_chrdev_region(irq.devid, IRQ_CNT);
	device_destroy(irq.class, irq.devid);
	class_destroy(irq.class);
}

module_init(irq_init);
module_exit(airq_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("rodney");