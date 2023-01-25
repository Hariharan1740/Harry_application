#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/init.h>
#include<linux/kdev_t.h>
#include<linux/fs.h>
#include<linux/err.h>
#include<linux/uaccess.h>
#include<linux/device.h>
#include<linux/cdev.h>
#include<linux/gpio.h>

#define LED 3
#define GPIO_LED 0
 
dev_t dev_major=0;
dev_t led_red=0;
dev_t led_yellow=0;
dev_t led_green=0;
int major_num=0;
int minor_num=0;
int GPIO=0;
int index_pos;

static struct class *led_dev_class;
static struct cdev led_cdev[LED];
static int __init led_driver_init(void);
static void __exit led_driver_exit(void);

static int led_dev_open(struct inode *led_inode, struct file *file);
static int led_dev_release(struct inode *led_inode, struct file *file);
static ssize_t led_dev_read(struct file *led, char __user *buffer, size_t length, loff_t *offset);
static ssize_t led_dev_write(struct file *led, const char *buffer, size_t length, loff_t *offset);

static struct file_operations led_ops=
{
	.owner= THIS_MODULE,
	.open= led_dev_open,
	.read= led_dev_read,
	.write= led_dev_write,
	.release= led_dev_release:
};

static int led_dev_open(struct inode *led_inode, struct file *file)
{
	minor_num=iminor(file_inode(file));
	pr_info("Minor number for particular file: %d\n",minor_num);
	pr_info("Device File Opened...\n");

	if(minor_num==0)
	{
		GPIO=4;
	}
	else if(minor_num==1)
	{
		GPIO=27;
	}
	else
	{
		GPIO=22;
	}
	if(gpio_is_valid(GPIO)==false)
	{
		pr_err("GPIO %d is not valid\n", GPIO);
		gpio_free(GPIO);
	}
	if(gpio_request(GPIO,"GPIO")<0)
	{
		pr_err("ERROR: GPIO %d request\n",GPIO);
		gpio_free(GPIO);
	}
	gpio_direction_output(GPIO,0);
	return 0;
}
static ssize_t led_dev_read(struct file *led, char __user *buffer, size_t length, loff_t *offset)
{
	uint8_t gpio_state=0;
	gpio_state = gpio_get_value(GPIO);

	length=1;
	if(copy_to_user(buffer,&gpio_state,length)>0)
	{
		pr_err("ERROR: Not all bytes copied to user\n");
	}
	pr_info("Read Function : GPIO = %d\n", gpio_state);
	return 0;	
}

static ssize_t led_dev_write(struct file *led, const char __user *buffer, size_t length, loff_t *offset)
{

	uint8_t new_buffer[10]={0};
	if(copy_from_user(new_buffer,buffer,length)>0)
	{
		pr_err("ERROR: Not all the bytes copied from user\n");
	}
	pr_info("Write Function : GPIO Set =%c\n",new_buffer[0]);
	if(new_buffer[0]=='1')
	{
		gpio_set_value(GPIO,1);
	}
	else if(new_buffer[0]=='0')
	{
		gpio_set_value(GPIO,0);
	}
	else
	{
		pr_err("UNKNOWN COMMAND : Please provide either 1 or 0\n");
	}
	return length;
}

static int led_dev_release(struct inode *led_inode, struct file *file)
{
	gpio_free(GPIO);
	pr_info("Device File Closed\n");
	return 0;
}

static int __init led_driver_init(void)
{

	if((alloc_chrdev_region(&dev_major,0,LED,"LED_DEVICE"))<0)
	{
		pr_err("Cannot Allocate the Major number\n");
		unregister_chrdev_region(dev_major,LED);
		return -1;
	}
	major_num=MAJOR(dev_major);
	led_red=MKDEV(major_num,0);
	led_yellow=MKDEV(major_num,1);
	led_green=MKDEV(major_num,2);

	pr_info("Major Red_led = %d Minor Red_led = %d\n",MAJOR(led_red),MINOR(led_red));
	pr_info("Major Yellow_led = %d Minor yellow_led = %d\n",MAJOR(led_yellow),MINOR(led_yellow));
	pr_info("Major Green_led = %d Minor Green_led = %d\n", MAJOR(led_green),MINOR(led_green));

	for(index_pos=0;index_pos<LED;index_pos++)
	{
		cdev_init(&led_cdev[index_pos],&led_ops);
	}

	if((cdev_add(&led_cdev[0],led_red,1))<0)
	{
		pr_err("Cannot add the red_led device to the system\n");
		cdev_del(&led_cdev[0]);
		return -1;
	}
	if((cdev_add(&led_cdev[1],led_yellow,1))<0)
	{
		pr_err("Cannot add the yellow_led device to the system\n");
		cdev_del(&led_cdev[1]);
		return -1;
	}
	if((cdev_add(&led_cdev[2],led_green,1))<0)
	{
		pr_err("Cannot add the green_led device to the system\n");
		cdev_del(&led_cdev[2]);
		return -1;
	}
	
	if(IS_ERR(led_dev_class=class_create(THIS_MODULE,"LED_DEVICE_CLASS")))
	{
		pr_err("Cannot create the struct class\n");
		class_destroy(led_dev_class);
		return -1;
	}
	if(IS_ERR(device_create(led_dev_class,NULL,led_red,NULL,"GPIO_First_led")))
	{
		pr_err("Cannot create the Device\n");
		device_destroy(led_dev_class,led_red);
		return -1;
	}
	if(IS_ERR(device_create(led_dev_class,NULL,led_yellow,NULL,"GPIO_Second_led")))
	{
		pr_err("Cannot create the Device\n");
		device_destroy(led_dev_class,led_yellow);
	return -1;
	}
	if(IS_ERR(device_create(led_dev_class,NULL,led_green,NULL,"GPIO_Third_led")))
	{
		pr_err("Cannot create the Device\n");
		device_destroy(led_dev_class,led_green);
		return -1;
	}

	pr_info("Driver Inserted Successfully\n");
	return 0;
}	
static void __exit led_driver_exit(void)
{

	device_destroy(led_dev_class,led_red);
	device_destroy(led_dev_class,led_yellow);
	device_destroy(led_dev_class,led_green);
	class_destroy(led_dev_class);

	for(index_pos=0;index_pos<LED;index_pos++)
	{
		cdev_del(&led_cdev[index_pos]);
	}
	unregister_chrdev_region(dev_major,LED);
	pr_info("Device Driver Removed\n");
}

module_init(led_driver_init);
module_exit(led_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hariharan V");
MODULE_DESCRIPTION("GPIO DRIVER FOR LED");

