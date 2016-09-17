#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

MODULE_AUTHOR("Ryuichi Ueda");
MODULE_DESCRIPTION("driver for LED control");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

static dev_t dev;
static struct cdev cdv;
static struct class *cls = NULL;
static spinlock_t spn_lock;
static int access_num = 0;

static ssize_t led_write(struct file* filp, const char* buf, size_t count, loff_t* pos)
{
	printk(KERN_INFO "led_write is called\n");
        return 1;
}

static int led_open(struct inode* inode, struct file* filp)
{
	spin_lock(&spn_lock);

	if(access_num){
		spin_unlock(&spn_lock);
		return -EBUSY;
	}

	access_num++;
	spin_unlock(&spn_lock);

	return 0;
}

static int led_release(struct inode* inode, struct file* filp)
{
	spin_lock(&spn_lock);
	access_num--;
	spin_unlock(&spn_lock);

	return 0;
}

static struct file_operations led_fops =
{
	owner   : THIS_MODULE,
	write   : led_write,
	open    : led_open,
	release : led_release,
};

static int __init init_mod(void)
{
	int retval;
	retval =  alloc_chrdev_region(&dev, 0, 1, "led");
	if(retval < 0){
		printk(KERN_ERR "alloc_chrdev_region failed.\n");
		return retval;
	}
	printk(KERN_INFO "%s is loaded. major:%d\n",__FILE__,MAJOR(dev));

	cdev_init(&cdv, &led_fops);
	retval = cdev_add(&cdv, MKDEV(MAJOR(dev),0), 1);
	if(retval < 0){
		printk(KERN_ERR "cdev_add failed. major:%d, minor:0\n",MAJOR(dev));
		return retval;
	}

	cls = class_create(THIS_MODULE,"myled");
	if(IS_ERR(cls)){
		printk(KERN_ERR "class_create failed.");
		return PTR_ERR(cls);
	}
	device_create(cls, NULL, MKDEV(MAJOR(dev),0), NULL, "myled0");

	return 0;
}

static void __exit cleanup_mod(void)
{
	cdev_del(&cdv);
	device_destroy(cls, MKDEV(MAJOR(dev),0));
	class_destroy(cls);
	unregister_chrdev_region(dev, 1);
	printk(KERN_INFO "%s is unloaded. major:%d\n",__FILE__,MAJOR(dev));
}

module_init(init_mod);
module_exit(cleanup_mod);
