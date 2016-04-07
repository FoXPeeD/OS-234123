
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
	
	char tmpName[256] = {0};
	if (access_ok(VERIFY_READ, name, sizeof(char)*(name_len + 1)))
	{
		unsigned int not_copied = copy_from_user(tmpName, name, sizeof(char)*(name_len+1));
		if (not_copied)
			return -EINVAL;
		name = (char*)tmpName;
	}
	
	list_for_each(ptr, &blacklist_head)
	{
		entry = list_entry(ptr, struct blacklist_programs_t, blacklist_member);
		if (strcmp(entry->blocked_name, name) == 0)
		{
			printk("Blocked %d from running\n", name);
			return 1;
		}
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
	
	INIT_LIST_HEAD(&new->blacklist_member);
	
	if (!access_ok(VERIFY_READ,name,sizeof(char)*(name_len+1)))
		return -EINVAL;
	
	
	unsigned int not_copied = copy_from_user(new->blocked_name, name, sizeof(char)*(name_len+1));
	if (not_copied)
	{
		kfree(new);
		return -EINVAL;
	}
	
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
	
	if (!access_ok(VERIFY_READ,name,sizeof(char)*(name_len+1)))
		return -EINVAL;
	
	char tmpName[256] = {0};
	unsigned int not_copied = copy_from_user(tmpName, name, sizeof(char)*(name_len+1));
	if (not_copied)
		return -EINVAL;
	
	list_for_each_safe(ptr, ptr2, &blacklist_head)
	{
		entry = list_entry(ptr, struct blacklist_programs_t, blacklist_member);
		if (strcmp(entry->blocked_name, tmpName) == 0)
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
	
	task_t* pid_struct = find_task_by_pid(pid);
	
	int min = n <= pid_struct->total_blocked ? n : pid_struct->total_blocked;
	
	if (!access_ok(VERIFY_WRITE, log, sizeof(char)*256*min))
		return -EFAULT;
	
	
	struct list_head *ptr = &pid_struct->blocked_head;
	struct blacklist_programs_t *entry;
	int iter_log = 0;
	list_for_each(ptr, &(pid_struct->blocked_head))
	{
		entry = list_entry(ptr, struct blocked_programs_t, list_member);
		unsigned int not_copied = copy_to_user(log[iter_log], entry->blocked_name, sizeof(char)*256);
		if (not_copied)
			return -EFAULT-1;
	
		iter_log++;
		if (iter_log >= min)
			return iter_log;
	}
	
	return pid_struct->total_blocked; // aka: return 0;
}
