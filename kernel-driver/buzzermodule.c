#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <asm/io.h>
#include <linux/timer.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/mm.h>

#include <linux/io.h>

#include "utils.h"



#define DEVICE_NAME "buzzer0"
#define CLASS_NAME "buzzerClass"


MODULE_LICENSE("GPL");


static struct class* buzzerDevice_class = NULL;
static struct device* buzzerDevice_device = NULL;
static dev_t buzzerDevice_majorminor;
static struct cdev c_dev;  // Character device structure


static struct class *s_pDeviceClass;
static struct device *s_pDeviceObject;
struct GpioRegisters *s_pGpioRegisters;

static const int BuzzerGpioPin = 12;

ssize_t buzzer_device_write(struct file *pfile, const char __user *pbuff, size_t len, loff_t *off) { 
	struct GpioRegisters *pdev; 
	
	pr_alert("%s: called (%u)\n",__FUNCTION__,len);

	
	if(unlikely(pfile->private_data == NULL))
		return -EFAULT;

	pdev = (struct GpioRegisters *)pfile->private_data;
	if (pbuff[0]=='0')
		SetGPIOOutputValue(pdev, BuzzerGpioPin, 0);
	else
		SetGPIOOutputValue(pdev, BuzzerGpioPin, 1);
	return len;
}

ssize_t buzzer_device_read(struct file *pfile, char __user *p_buff,size_t len, loff_t *poffset){
	pr_alert("%s: called (%u)\n",__FUNCTION__,len);
	return 0;
}

int buzzer_device_close(struct inode *p_inode, struct file * pfile){
	
	pr_alert("%s: called\n",__FUNCTION__);
	pfile->private_data = NULL;
	return 0;
}


int buzzer_device_open(struct inode* p_indode, struct file *p_file){

	pr_alert("%s: called\n",__FUNCTION__);
	p_file->private_data = (struct GpioRegisters *) s_pGpioRegisters;
	return 0;
	
}


static struct file_operations buzzerDevice_fops = {
	.owner = THIS_MODULE,
	.write = buzzer_device_write,
	.read = buzzer_device_read,
	.release = buzzer_device_close,
	.open = buzzer_device_open,
};

static int __init buzzerModule_init(void) {
	int ret;
	struct device *dev_ret;

	pr_alert("%s: called\n",__FUNCTION__);

	if ((ret = alloc_chrdev_region(&buzzerDevice_majorminor, 0, 1, DEVICE_NAME)) < 0) {
		return ret;
	}

	if (IS_ERR(buzzerDevice_class = class_create(CLASS_NAME))) {
		unregister_chrdev_region(buzzerDevice_majorminor, 1);
		return PTR_ERR(buzzerDevice_class);
	}
	if (IS_ERR(dev_ret = device_create(buzzerDevice_class, NULL, buzzerDevice_majorminor, NULL, DEVICE_NAME))) {
		class_destroy(buzzerDevice_class);
		unregister_chrdev_region(buzzerDevice_majorminor, 1);
		return PTR_ERR(dev_ret);
	}

	cdev_init(&c_dev, &buzzerDevice_fops);
	c_dev.owner = THIS_MODULE;
	if ((ret = cdev_add(&c_dev, buzzerDevice_majorminor, 1)) < 0) {
		printk(KERN_NOTICE "Error %d adding device", ret);
		device_destroy(buzzerDevice_class, buzzerDevice_majorminor);
		class_destroy(buzzerDevice_class);
		unregister_chrdev_region(buzzerDevice_majorminor, 1);
		return ret;
	}


	s_pGpioRegisters = ioremap(GPIO_BASE, sizeof(struct GpioRegisters));
	
	pr_alert("map to virtual adresse: 0x%x\n", (unsigned)s_pGpioRegisters);
	
	SetGPIOFunction(s_pGpioRegisters, BuzzerGpioPin, 0b001); //Output

	return 0;
}

static void __exit buzzerModule_exit(void) {
	
	pr_alert("%s: called\n",__FUNCTION__);
	
	SetGPIOFunction(s_pGpioRegisters, BuzzerGpioPin, 0); //Configure the pin as input
	iounmap(s_pGpioRegisters);
	cdev_del(&c_dev);
	device_destroy(buzzerDevice_class, buzzerDevice_majorminor);
	class_destroy(buzzerDevice_class);
	unregister_chrdev_region(buzzerDevice_majorminor, 1);
}

module_init(buzzerModule_init);
module_exit(buzzerModule_exit);

