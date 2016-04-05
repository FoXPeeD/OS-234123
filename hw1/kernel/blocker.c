
// #BENITZIK: Our system calls. KernelSpace!!

#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <asm-i386/errno.h>


// New struct for list of blocked attempts.
struct blacklist_programs_t {
	struct list_head blacklist_member;
	char blocked_name[256];
};

LIST_HEAD(blacklist_head);
int total_blocked = 0;

int sys_is_program_blocked(const char *name, unsigned int name_len)
{
	printk("In Function: sys_is_program_blocked %s %d", name, name_len);
	
	struct list_head *ptr;
	struct blacklist_programs_t *entry;
	
	if ((name == NULL) || (name_len == 0))
		return -EINVAL;
	
	list_for_each(ptr, &blacklist_head)
	{
		entry = list_entry(ptr, struct blacklist_programs_t, blacklist_member);
		if (strcmp(entry->blocked_name, name))
			return 1;
	}
	
	return 0;
}


int sys_block_program(const char *name, unsigned int name_len)
{
	printk("In Function: sys_unblock_program %s %d", name, name_len);
	
	if ((name == NULL) || (name_len == 0))
		return -EINVAL;

	if (sys_is_program_blocked(name, name_len))
		return 1;
	
	struct blacklist_programs_t *new = (struct blacklist_programs_t *)kmalloc(sizeof(struct blacklist_programs_t), 0);
	if (!new)
		return -ENOMEM;
	
	strcpy(new->blocked_name, name);
	list_add_tail(&(new->blacklist_member), &blacklist_head);
	total_blocked++;
	return 0;
}

int sys_unblock_program(const char *name, unsigned int name_len)
{
	if ((name == NULL) || (name_len == 0))
		return -EINVAL;

	struct list_head *ptr;
	struct list_head *ptr2;
	struct blacklist_programs_t *entry;
	
	printk("In Function: sys_unblock_program %s %d", name, name_len);

	list_for_each_safe(ptr, ptr2, &blacklist_head)
	{
		entry = list_entry(ptr, struct blacklist_programs_t, blacklist_member);
		if (strcmp(entry->blocked_name, name))
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
	printk("In Function: sys_get_blocked_count");
	
	return total_blocked;
}

int sys_get_forbidden_tries (int pid, char log[][256], unsigned int n)
{
	printk("In Dummy Function: sys_get_forbidden_tries %d %d", pid, n);	// Didnt print log

	if (n == 0)
		return -EINVAL;
	
	if (!find_task_by_pid(current->pid))
		return -ESRCH;
	
	// TODO: Body of function
	if (1)
		return -EFAULT;
	
	return find_task_by_pid(current->pid)->total_blocked;
}
