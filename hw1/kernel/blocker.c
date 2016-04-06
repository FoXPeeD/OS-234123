
// #BENITZIK: Our system calls. KernelSpace!!

#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <asm-i386/errno.h>
#include <asm-i386/uaccess.h>


// New struct for list of blocked attempts.
struct blacklist_programs_t {
	struct list_head blacklist_member;
	char blocked_name[256];
};

LIST_HEAD(blacklist_head);
int total_blocked = 0;

int sys_is_program_blocked(const char *name, unsigned int name_len)
{
	
	struct list_head *ptr;
	struct blacklist_programs_t *entry;
	
	if ((name == NULL) || (name_len == 0))
		return -EINVAL;
	if(! access_ok(VERIFY_READ,name,sizeof(char)*(name_len+1))
	{
		return -EINVAL;
	}
	
		if(! access_ok(VERIFY_READ,name,sizeof(char)*(name_len+1))
	{
		return -EINVAL;
	}
	char tmpName[256] = {0};
	unsigned int not_copied = copy_from_user(tmpName,name,sizeof(char)*(name_len+1));
	if(not_copied!=0)
	{
		return -EFAULT
	}
	
	list_for_each(ptr, &blacklist_head)
	{
		entry = list_entry(ptr, struct blacklist_programs_t, blacklist_member);
		if (strcmp(entry->blocked_name, tmpName))
			return 1;
	}
	
	return 0;
}


int sys_block_program(const char *name, unsigned int name_len)
{
	printk("In Function: sys_block_program %s %d\n", name, name_len);
	
	if ((name == NULL) || (name_len == 0))
		return -EINVAL;

	if (sys_is_program_blocked(name, name_len))
		return 1;
	
	struct blacklist_programs_t *new = (struct blacklist_programs_t *)kmalloc(sizeof(struct blacklist_programs_t), GFP_KERNEL);
	if (!new)
		return -ENOMEM;
	if(! access_ok(VERIFY_READ,name,sizeof(char)*(name_len+1))
	{
		kfree(new);
		return -EINVAL;
	}
	unsigned int not_copied = copy_from_user(new->blocked_name,name,sizeof(char)*(name_len+1));
	list_add_tail(&(new->blacklist_member), &blacklist_head);
	total_blocked++;

	return 0;
}

int sys_unblock_program(const char *name, unsigned int name_len)
{
	printk("In Function: sys_unblock_program %s %d\n", name, name_len);
	
	if ((name == NULL) || (name_len == 0))
		return -EINVAL;

	struct list_head *ptr;
	struct list_head *ptr2;
	struct blacklist_programs_t *entry;
	
	if(! access_ok(VERIFY_READ,name,sizeof(char)*(name_len+1))
	{
		return -EINVAL;
	}
	char tmpName[256] = {0};
	unsigned int not_copied = copy_from_user(tmpName,name,sizeof(char)*(name_len+1));
	if(not_copied!=0)
	{
		return -EFAULT
	}
		
	
	list_for_each_safe(ptr, ptr2, &blacklist_head)
	{
		entry = list_entry(ptr, struct blacklist_programs_t, blacklist_member);
		if (strcmp(entry->blocked_name, tmpName))
		{
			list_del(&entry->blacklist_member);
			total_blocked--;
			return 0;
		}
	}

	return 1;
}

int sys_get_blocked_count(void)
{
	printk("In Function: sys_get_blocked_count\n");
	
	return total_blocked;
}

int sys_get_forbidden_tries (int pid, char log[][256], unsigned int n)
{
	printk("In Function: sys_get_forbidden_tries %d %d\n", pid, n);	// Didnt print log

	if (n == 0)
		return -EINVAL;
	
	if (!find_task_by_pid(pid))
		return -ESRCH;
	
	// TODO: Body of function
	if (!access_ok(VERIFY_WRITE,log,sizeof(char)*256*n))
		return -EFAULT;
	
	
	task_t* pid_struct = find_task_by_pid(pid);
	struct list_head *ptr = pid_struct->blocked_head;
	struct blacklist_programs_t *entry;
	int iter_log = 0;
	list_for_each(ptr, &pid_struct->blacklist_head)
	{
		entry = list_entry(ptr, struct blacklist_programs_t, blacklist_member);
		copy_to_user(log[iter_log][256],entry->blocked_name,sizeof(char)*256);
		iter_log++;
		if( (iter_log>=total_blocked) || (iter_log>=n) )
		{
			return iter_log;
		}
	}
	
	
	return find_task_by_pid(pid)->total_blocked;
}
