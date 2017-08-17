#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>

#include <asm/uaccess.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/arch/regs-gpio.h>

DECLARE_WAIT_QUEUE_HEAD(buttons_wq);

static dev_t dev;

unsigned char ev_key;

struct pin_desc{
	int irq;
	char *name;
	unsigned int pin;
	unsigned int key_val;
};

struct pin_desc pins_desc[4] = {
	{IRQ_EINT0,  "S2", S3C2410_GPF0,   1},
	{IRQ_EINT2,  "S3", S3C2410_GPF2,   2},
	{IRQ_EINT11, "S4", S3C2410_GPG3,   3},
	{IRQ_EINT19, "S5",  S3C2410_GPG11, 4},
};

struct timer_list buttons_timer;

static irqreturn_t buttons_isr(int ire, void *dev_id)
{
	buttons_timer.data = (unsigned long)dev_id;

	mod_timer(&buttons_timer, jiffies + HZ/100);

	return IRQ_HANDLED;
}

static void buttons_timer_func(unsigned long data)
{
	struct pin_desc *pt = (struct pin_desc*)data;

	if(NULL == pt)
		return;

	ev_key = pt->key_val;

	wake_up_interruptible(&buttons_wq);
}

static int buttons_open(struct inode *inode, struct file *filp)
{
	int i = 0;
	int err = 0;

	//初始化定时器
	init_timer(&buttons_timer);
	buttons_timer.function = buttons_timer_func;
	add_timer(&buttons_timer);

	//注册中断
	for(i = 0; i < 4; i++)
	{
		err = request_irq(pins_desc[i].irq, buttons_isr, IRQF_TRIGGER_FALLING, pins_desc[i].name, &pins_desc[i]);

		if(-1 == err) printk("failed to requett %s\n", pins_desc[i].name);
	}

	return 0;
}

static ssize_t buttons_read(struct file *filp, char __user *buff, size_t count, loff_t *offset)
{
	wait_event_interruptible(buttons_wq, ev_key);

	copy_to_user(buff, &ev_key, 1);

	ev_key = 0;

	return 1;
}

static int buttons_close(struct inode *inode, struct file *filp)
{
	int i = 0;

	//删除定时器
	del_timer(&buttons_timer);

	//注销中断
	for(i = 0; i < 4; i++)
	{
		free_irq(pins_desc[i].irq, &pins_desc[i]);
	}

	return 0;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = buttons_open,
	.read = buttons_read,
	.release = buttons_close,
};

static struct cdev *buttons_cdev;

static struct class *buttons_class;

static int __init buttons_init(void)
{
	//申请设备号
	alloc_chrdev_region(&dev, 0, 1, "buttons");

	//添加字符设备
	buttons_cdev = cdev_alloc();
	cdev_init(buttons_cdev, &fops);
	cdev_add(buttons_cdev, dev, 1);

	//创建类
	buttons_class = class_create(THIS_MODULE, "buttons");
	class_device_create(buttons_class, NULL, dev, NULL, "buttons");
	
	return 0;
}

static void __exit buttons_exit(void)
{
	//删除类
	class_destroy(buttons_class);

	//卸载字符设备
	cdev_del(buttons_cdev);

	//释放设备号
	unregister_chrdev_region(dev, 1);
}

MODULE_LICENSE("GPL");
module_init(buttons_init);
module_exit(buttons_exit);

