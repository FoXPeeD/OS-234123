#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <errno.h>
#include <linux/capability.h>

//for defining __u32 and struct rand_pool_info
#include <linux/types.h>
#include <linux/random.h>

/* XXX define this to the major number of your device */
#define SRANDOM_MAJOR 62
#define MAX_ENTROPY 4096
#define CHUNK_SIZE 64
#define POOL_SIZE 512
#define MIN_ENTROPY_TO_READ 8
#define READ_CHUNK 20


#DEFINE RNDGETENTCNT _IOR(SRANDOM_MAJOR,0,int)
#DEFINE RNDCLEARPOOL _IO(SRANDOM_MAJOR,1)
#DEFINE RNDADDENTROPY _IOW(SRANDOM_MAJOR,2,struct rand_pool_info)

char pooldata[POOL_SIZE];
unsigned int entropy_count;
DECLARE_WAIT_QUEUE_HEAD(my_waitqueue);

int srandom_get_count(int *p)
{
	if( !access_ok(VERIFY_WRITE,p,sizeof(int)) ) 
		return -EFAULT;
	if ( !copy_to_user(p,&entropy_count),sizeof(int) ) 
		return -EFAULT;
	return 0;
}

int srandom_clear_pool(void)
{
	if(!capable(CAP_SYS_ADMIN)) 
		return -EPERM;
	entropy_count = 0;
	return 0;
}


int srandom_add_antropy (struct rand_pool_info *p)
{
	if(!capable(CAP_SYS_ADMIN)) 
		return -EPERM;
	
	char *my_p = kmalloc(sizeof(rand_pool_info), GFP_KERNEL);
	if( !my_p ) 
		return -NOMEM;
	if( !copy_from_user(my_p,p,sizeof(rand_pool_info)) )  
		return -EFAULT;
	if(my_p->buf_size==0) 
		return -EFAULT;
	
	if(my_p->entropy_count < 0) 
		return -EINVAL;
	
	char *my_buf = kmalloc(sizeof(rand_pool_info)+my_p->buf_size, GFP_KERNEL);
	if( !my_buf ) 
		return -NOMEM;
	if( !copy_from_user(my_buf,p,sizeof(rand_pool_info)+my_p->buf_size) )  
		return -EFAULT;

	int count = my_p->buf_size;
	int index = 0;
	while(count > CHUNK_SIZE){
		mix( (my_buf->buf)[index] ,CHUNK_SIZE,pooldata);
		index += CHUNK_SIZE;
		count -= CHUNK_SIZE;
	}
	mix((my_buf->buf)[index],count,pooldata);

	entropy_count += my_p->entropy_count;
	if (entropy_count > MAX_ENTROPY) 
		entropy_count = MAX_ENTROPY;
	kfree(my_buf);
	kfree(my_buf);
	
	if( count >= MIN_ENTROPY_TO_READ ) wake_up_interruptible(&my_waitqueue);
	return count;
	
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
	if(count == 0) 
		return 0;
	while(MIN_ENTROPY_TO_READ <= entropy_count)
	{
		wait_event_interruptible(&my_waitqueue, count >= MIN_ENTROPY_TO_READ);
		if (signal_pending(current)) 
			return -ERESTARTSYS;
	}
	int E = entropy_count/MIN_ENTROPY_TO_READ;
	int n = 0;
	if (count > E) 
		n = E;
	else 
		n = count;
	
	entropy_count = entropy_count - (n * MIN_ENTROPY_TO_READ);
	if ( entropy_count < 0 ) 
		entropy_count = 0;

	char *my_buf = kmalloc(count, GFP_KERNEL);
	if( !my_buf ) 
		return -NOMEM;
	
	int left = n;
	int index = 0;
	
	while(left > READ_CHUNK)
	{
		hash_pool(pooldata, &(my_buf[index]));
		mix (&(my_buf[index]), READ_CHUNK, pooldata);
		
		left -= READ_CHUNK;
		index += READ_CHUNK;
	}
	char tmp[READ_CHUNK] = {0};
	hash_pool(pooldata, tmp);
	mix (tmp, READ_CHUNK, pooldata);
	memcpy(&(my_buf[index]),tmp,left);
	
	if( !copy_from_user(my_buf,buf,n) )  
		return -EFAULT;

	return n;
}


static ssize_t srandom_write (struct file *file, const char *buf,
		size_t count, loff_t *ppos)
{
	if(count==0) 
		return -EFAULT;
	char *my_buf = kmalloc(count, GFP_KERNEL);
	if( !my_buf ) 
		return -NOMEM;
	if( !copy_from_user(my_buf,buf,count) )  
		return -EFAULT;
	
	int index = 0;
	while(count > CHUNK_SIZE){
		mix(my_buf[index],CHUNK_SIZE,pooldata);
		index += CHUNK_SIZE;
		count -= CHUNK_SIZE;
	}
	mix(my_buf[index],count,pooldata);

	kfree(my_buf);

	return count;
}

//TODO:
static int srandom_ioctl (struct inode *inode, struct file *file,
		unsigned int cmd, unsigned long arg)
{
	if ( cmd == RNDGETENTCNT ) 
		return srandom_get_count(arg);
	if ( cmd == RNDCLEARPOOL ) 
		return srandom_clear_pool();
	if ( cmd == RNDADDENTROPY ) 
		return srandom_add_antropy(arg);

	return -ENOTTY;
}

static struct file_operations srandom_fops = {
	owner: THIS_MODULE,
	open: srandom_open,
	release: srandom_release,
	read: srandom_read,
	write: srandom_write,
	ioctl: srandom_ioctl,
};

static int __init init_srandom (void)
{
	int retval;

	/* XXX initialize global variables */

	retval = register_chrdev (SRANDOM_MAJOR, "srandom", &srandom_fops);
	if (retval < 0)
		return retval;
	
	memset(pooldata,0,POOL_SIZE);
	retval = RNDCLEARPOOL ();
	if (retval < 0)
		return retval; //TODO: check which error to return
	
	
	return 0;
}

static void __exit cleanup_srandom (void)
{
	unregister_chrdev (SRANDOM_MAJOR, "srandom");

	/* XXX cleanup, release memory, etc. */
}

module_init(init_srandom);
module_exit(cleanup_srandom);

MODULE_LICENSE("GPL");



