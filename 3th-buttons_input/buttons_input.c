#include <linux/module.h>
#include <linux/version.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/irq.h>

#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/arch/regs-gpio.h>

/* 
 * 1. 分配input_dev结构
 * 2. 设置事件类型
 * 3. 注册inpit_dev
 * 4. 硬件操作
 */

MODULE_LICENSE("GPL");

struct pin_desc{
	int irq;
	char *name;
	unsigned int pin;
	unsigned int key_val;
};

struct pin_desc pins_desc[4] = {
	{IRQ_EINT0,  "S2", S3C2410_GPF0,   KEY_L},
	{IRQ_EINT2,  "S3", S3C2410_GPF2,   KEY_S},
	{IRQ_EINT11, "S4", S3C2410_GPG3,   KEY_ENTER},
	{IRQ_EINT19, "S5",  S3C2410_GPG11, KEY_LEFTSHIFT},
};

struct timer_list buttons_timer;

static struct input_dev *buttons_input_dev;

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

	input_event(buttons_input_dev, EV_KEY, pt->key_val, 1);
	//input_event(buttons_input_dev, EV_KEY, pt->key_val, 0);
	input_sync(buttons_input_dev);
}

static int __init buttons_init(void)
{
	int i = 0;
	int err = 0;

	//分配input_dev结构
	buttons_input_dev = input_allocate_device();

	//设置事件类型
	set_bit(EV_KEY, buttons_input_dev->evbit);
	set_bit(KEY_L, buttons_input_dev->keybit);
	set_bit(KEY_S, buttons_input_dev->keybit);
	set_bit(KEY_ENTER, buttons_input_dev->keybit);
	set_bit(KEY_LEFTSHIFT, buttons_input_dev->keybit);

	//注册
	input_register_device(buttons_input_dev);

	//初始化定时器
	init_timer(&buttons_timer);
	buttons_timer.function = buttons_timer_func;
	add_timer(&buttons_timer);

	//注册中断
	for(i = 0; i < 4; i++)
	{
		err = request_irq(pins_desc[i].irq, buttons_isr, IRQT_FALLING, pins_desc[i].name, &pins_desc[i]);
		//err = request_irq(pins_desc[i].irq, buttons_isr, IRQT_BOTHEDGE, pins_desc[i].name, &pins_desc[i]);

		if(-1 == err) printk("failed to requett %s\n", pins_desc[i].name);
	}
	
	return 0;
}

static void __exit buttons_exit(void)
{
	int i = 0;

	//释放中断
	for(i = 0; i < 4; i++)
	{
		free_irq(pins_desc[i].irq, &pins_desc[i]);
	}

	//删除定时器
	del_timer(&buttons_timer);

	//注销输入设备
	input_unregister_device(buttons_input_dev);

	//释放结构
	input_free_device(buttons_input_dev);
}

module_init(buttons_init);
module_exit(buttons_exit);


