#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>

/* XXX define this to the major number of your device */
#define SIMPLE_MAJOR 110

struct simple_data {
	/* XXX put your "private data" here */
};

/* XXX put your global variables here */

static int simple_open (struct inode *inode, struct file *file)
{
	struct simple_data *data;

	/* XXX implement your open() method */

	data = kmalloc (sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	/* XXX initialize *data */

	file->private_data = data;

	return 0;
}

static int simple_release (struct inode *inode, struct file *file)
{
	struct simple_data *data = file->private_data;

	/* XXX implement your release() method */

	kfree (data);

	return 0;
}

static ssize_t simple_read (struct file *file, char *buf,
		size_t count, loff_t *ppos)
{
	struct simple_data *data = file->private_data;

	/* XXX implement your read() method */

	return 0;
}

static ssize_t simple_write (struct file *file, const char *buf,
		size_t count, loff_t *ppos)
{
	struct simple_data *data = file->private_data;

	/* XXX implement your write() method */

	return count;
}

static int simple_ioctl (struct inode *inode, struct file *file,
		unsigned int cmd, unsigned long arg)
{
	struct simple_data *data = file->private_data;

	/* XXX implement your ioctl() method */

	return -ENOTTY;
}

static struct file_operations simple_fops = {
	owner: THIS_MODULE,
	open: simple_open,
	release: simple_release,
	read: simple_read,
	write: simple_write,
	llseek: no_llseek,
	ioctl: simple_ioctl,
};

static int __init init_simple (void)
{
	int retval;

	/* XXX initialize global variables */

	retval = register_chrdev (SIMPLE_MAJOR, "simpledev", &simple_fops);
	if (retval < 0)
		return retval;

	return 0;
}

static void __exit cleanup_simple (void)
{
	unregister_chrdev (SIMPLE_MAJOR, "simpledev");

	/* XXX cleanup, release memory, etc. */
}

module_init(init_simple);
module_exit(cleanup_simple);

MODULE_LICENSE("GPL");
