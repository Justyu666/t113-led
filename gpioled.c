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
/***************************************************************
Copyright ? ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
�ļ���		: gpioled.c
����	  	: ���ҿ�
�汾	   	: V1.0
����	   	: ����pinctrl��gpio��ϵͳ����LED�ơ�
����	   	: ��
��̳ 	   	: www.openedv.com
��־	   	: ����V1.0 2019/7/13 ���ҿ�����
***************************************************************/
#define GPIOLED_CNT			1		  	/* �豸�Ÿ��� */
#define GPIOLED_NAME		"gpioled"	/* ���� */
#define LEDOFF 				0			/* �ص� */
#define LEDON 				1			/* ���� */

/* gpioled�豸�ṹ�� */
struct gpioled_dev{
	dev_t devid;			/* �豸�� 	 */
	struct cdev cdev;		/* cdev 	*/
	struct class *class;	/* �� 		*/
	struct device *device;	/* �豸 	 */
	int major;				/* ���豸��	  */
	int minor;				/* ���豸��   */
	struct device_node	*nd; /* �豸�ڵ� */
	int led_gpio;			/* led��ʹ�õ�GPIO���		*/
};
struct gpioled_dev gpioled;	/* led�豸 */
/*
 * @description		: ���豸
 * @param - inode 	: ���ݸ�������inode
 * @param - filp 	: �豸�ļ���file�ṹ���и�����private_data�ĳ�Ա����
 * 					  һ����open��ʱ��private_dataָ���豸�ṹ�塣
 * @return 			: 0 �ɹ�;���� ʧ��
 */
static int led_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &gpioled; /* ����˽������ */
	return 0;
}
/*
 * @description		: ���豸��ȡ���� 
 * @param - filp 	: Ҫ�򿪵��豸�ļ�(�ļ�������)
 * @param - buf 	: ���ظ��û��ռ�����ݻ�����
 * @param - cnt 	: Ҫ��ȡ�����ݳ���
 * @param - offt 	: ������ļ��׵�ַ��ƫ��
 * @return 			: ��ȡ���ֽ��������Ϊ��ֵ����ʾ��ȡʧ��
 */
static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	return 0;
}
/*
 * @description		: ���豸д���� 
 * @param - filp 	: �豸�ļ�����ʾ�򿪵��ļ�������
 * @param - buf 	: Ҫд���豸д�������
 * @param - cnt 	: Ҫд������ݳ���
 * @param - offt 	: ������ļ��׵�ַ��ƫ��
 * @return 			: д����ֽ��������Ϊ��ֵ����ʾд��ʧ��
 */
static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue;
	unsigned char databuf[1];
	unsigned char ledstat;
	struct gpioled_dev *dev = filp->private_data;
	retvalue = copy_from_user(databuf, buf, cnt);
	if(retvalue < 0) {
		printk("kernel write failed!\r\n");
		return -EFAULT;
	}
	ledstat = databuf[0];		/* ��ȡ״ֵ̬ */
	if(ledstat == LEDON) {	
		gpio_set_value(dev->led_gpio, 0);	/* ��LED�� */
	} else if(ledstat == LEDOFF) {

		gpio_set_value(dev->led_gpio, 1);	/* �ر�LED�� */
	}
	return 0; 
}
/*
 * @description		: �ر�/�ͷ��豸
 * @param - filp 	: Ҫ�رյ��豸�ļ�(�ļ�������)
 * @return 			: 0 �ɹ�;���� ʧ��
 */
static int led_release(struct inode *inode, struct file *filp)
{
	return 0;
}
/* �豸�������� */
static struct file_operations gpioled_fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.read = led_read,
	.write = led_write,
	.release = 	led_release,
};
/*
 * @description	: �������ں���
 * @param 		: ��
 * @return 		: ��
 */
static int __init led_init(void)
{
	int ret = 0;
	ret = ret;
	/* ����LED��ʹ�õ�GPIO */
	/* 1����ȡ�豸�ڵ㣺gpioled */
	gpioled.nd = of_find_node_by_path("/gpioled");
	if(gpioled.nd == NULL) {
		printk("gpioled node not find!\r\n");
		return -EINVAL;
	} else {
		printk("gpioled node find!\r\n");
	} 
	/* 2�� ��ȡ�豸���е�gpio���ԣ��õ�LED��ʹ�õ�LED��� */
	gpioled.led_gpio = of_get_named_gpio(gpioled.nd, "gpios", 0);
	if(gpioled.led_gpio < 0) {
		printk("can't get led-gpio");
		return -EINVAL;
	}
	printk("led-gpio num = %d\r\n", gpioled.led_gpio);
	/* 3������GPIO1_IO03Ϊ�������������ߵ�ƽ��Ĭ�Ϲر�LED�� */
	ret = gpio_direction_output(gpioled.led_gpio, 0);
	if(ret < 0) {
		printk("can't set gpio!\r\n");
	} 
	/* ע���ַ��豸���� */
	/* 1�������豸�� */
	if (gpioled.major) {		/*  �������豸�� */
		gpioled.devid = MKDEV(gpioled.major, 0);
		register_chrdev_region(gpioled.devid, GPIOLED_CNT, GPIOLED_NAME);
	} else {						/* û�ж����豸�� */
		alloc_chrdev_region(&gpioled.devid, 0, GPIOLED_CNT, GPIOLED_NAME);	/* �����豸�� */
		gpioled.major = MAJOR(gpioled.devid);	/* ��ȡ����ŵ����豸�� */
		gpioled.minor = MINOR(gpioled.devid);	/* ��ȡ����ŵĴ��豸�� */
	}
	printk("gpioled major=%d,minor=%d\r\n",gpioled.major, gpioled.minor);	
	/* 2����ʼ��cdev */
	gpioled.cdev.owner = THIS_MODULE;
	cdev_init(&gpioled.cdev, &gpioled_fops);
	/* 3�����һ��cdev */
	cdev_add(&gpioled.cdev, gpioled.devid, GPIOLED_CNT);
	/* 4�������� */
	gpioled.class = class_create(THIS_MODULE, GPIOLED_NAME);
	if (IS_ERR(gpioled.class)) {
		return PTR_ERR(gpioled.class);
	}
	/* 5�������豸 */
	gpioled.device = device_create(gpioled.class, NULL, gpioled.devid, NULL, GPIOLED_NAME);
	if (IS_ERR(gpioled.device)) {
		return PTR_ERR(gpioled.device);
	}
	return 0;
}

static void __exit led_exit(void)
{
	/* ע���ַ��豸���� */
	cdev_del(&gpioled.cdev);/*  ɾ��cdev */
	unregister_chrdev_region(gpioled.devid, GPIOLED_CNT); /* ע���豸�� */
	device_destroy(gpioled.class, gpioled.devid);
	class_destroy(gpioled.class);
}
module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zuozhongkai");