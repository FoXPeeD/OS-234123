#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>

//for defining __u32 and struct rand_pool_info
#include <linux/types.h>
#include <linux/random.h>

/* XXX define this to the major number of your device */
#define SRANDOM_MAJOR 62


char pooldata[512];
unsigned int entropy_count;
DECLARE_WAIT_QUEUE_HEAD(my_waitqueue);


int RNDCLEARPOOL (void)
{
	if(!capable(CAP_SYS_ADMIN)) return -EPERM;
	entropy_count = 0;
	return 0;
}


RNDADDENTROPY (struct rand_pool_info *p)
{
	
	if( count>=8 ) wake_up_interruptible(&my_waitqueue);
}
//struct simple_data {
//	/* XXX put your "private data" here */
//};

/* XXX put your global variables here */

static int srandom_open (struct inode *inode, struct file *file)
{
	return 0;
}

static int srandom_release (struct inode *inode, struct file *file)
{
	return 0;
}

//TODO:
static ssize_t srandom_read (struct file *file, char *buf,
		size_t count, loff_t *ppos)
{
	struct simple_data *data = file->private_data;

	/* XXX implement your read() method */

	return 0;
}

//TODO: "loff_t *ppos"?
static ssize_t srandom_write (struct file *file, const char *buf,
		size_t count, loff_t *ppos)
{
	if(count==0) return -EFAULT;
	char *my_buf = kmalloc(count, GFP_KERNEL);
	if( !copy_from_user(my_buf,buf,count) )  return -EFAULT;
	
	int index = 0;
	while(count>64){
		mix(my_buf[index],64,pooldata);
		index += 64;
		count -= 64;
	}
	mix(my_buf[index],count,pooldata);

	kfree(my_buf);

	return count;
}

//TODO:
static int srandom_ioctl (struct inode *inode, struct file *file,
		unsigned int cmd, unsigned long arg)
{
	struct simple_data *data = file->private_data;

	/* XXX implement your ioctl() method */

	return -ENOTTY;
}

static struct file_operations srandom_fops = {
	owner: THIS_MODULE,
	open: srandom_open,
	release: srandom_release,
	read: srandom_read,
	write: srandom_write,
	llseek: no_llseek,
	ioctl: srandom_ioctl,
};

static int __init init_srandom (void)
{
	int retval;

	/* XXX initialize global variables */

	retval = register_chrdev (SIMPLE_MAJOR, "srandom", &srandom_fops);
	if (retval < 0)
		return retval;
	
	memset(pooldata,0,512);
	retval = RNDCLEARPOOL ();
	if (retval < 0)
		return retval; //TODO: check which error to return
	
	
	return 0;
}

static void __exit cleanup_srandom (void)
{
	unregister_chrdev (SIMPLE_MAJOR, "srandom");

	/* XXX cleanup, release memory, etc. */
}

module_init(init_srandom);
module_exit(cleanup_srandom);

MODULE_LICENSE("GPL");



