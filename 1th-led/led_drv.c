#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#define GPFCON 0x56000050

#define LED_MSK ((0x3 << 8) | (0x3 << 10) | (0x3 << 12))
#define LED_OUT ((0x1 << 8) | (0x1 << 10) | (0x1 << 12))
#define LEDDAT ((1 << 4) | (1 << 5) | (1 << 6))

volatile unsigned long *gpfcon;
volatile unsigned long *gpfdat;
dev_t dev_nu;
struct class *led_class;

static int led_open(struct inode *inode, struct file *filp)
{
	/* 设置端口为输出模式 */
	*gpfcon &= ~LED_MSK;
	*gpfcon |= LED_OUT;

	return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buff, size_t len, loff_t *loff)
{
	char dat = 0;

	/* 根据数据操作LED */
	copy_from_user(&dat, buff, 1);

	if(dat)
		*gpfdat &= ~LEDDAT;
	else
		*gpfdat |= LEDDAT;

	return 0;
}

static int led_close(struct inode *inode, struct file *filp)
{

	return 0;
}

struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.write = led_write,
	.release = led_close,
};
struct cdev *led_cdev;

static int __init led_init(void)
{
	/* 1. 申请设备号 */
	alloc_chrdev_region(&dev_nu, 0, 1, "led");

	/* 2. 分配cdev结构 */
	led_cdev = cdev_alloc();

	cdev_init(led_cdev, &led_fops);

	/* 3. 注册字符设备 */
	cdev_add(led_cdev, dev_nu, 1);

	/* 4. 地址映射 */
	gpfcon = (volatile unsigned long *)ioremap(GPFCON, 8);
	gpfdat = gpfcon + 1;

	/* 5. 创建类 */
	led_class = class_create(THIS_MODULE, "leds");

	/* 6. 创建设备 */
	class_device_create(led_class, NULL, dev_nu, NULL, "led1");

	return 0;
}

static void __exit led_exit(void)
{
	class_destroy(led_class);

	cdev_del(led_cdev);

	unregister_chrdev_region(dev_nu, 1);

	iounmap(gpfcon);
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");


