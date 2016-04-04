
// #BENITZIK: Our system calls. KernelSpace!!

#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <asm-i386/errno.h>

// New struct for list of blocked attempts.
struct blacklist_programs_t {
	struct list_head blacklist_member;
	char* blocked_name[256];
};

LIST_HEAD(blacklist_head);

int sys_is_program_blocked(const char *name, unsigned int name_len)
{
	printk("In Dummy Function: sys_is_program_blocked %s %d", name, name_len);
	return 3;
}


int sys_block_program(const char *name, unsigned int name_len)
{
	printk("In REAL Function: sys_unblock_program %s %d", name, name_len);
	
	if ((name == NULL) || (name_len == 0))
		return -EINVAL;

	if (sys_is_program_blocked(name, name_len))
		return 0;
	
	struct blacklist_programs_t *new = (struct blacklist_programs_t *)kmalloc(sizeof(struct blacklist_programs_t), 0);
	if (!new)
		return -ENOMEM;
	
	strcpy(new->blocked_name, name);
	//list_add_tail(new->list, blacklist_programs->list);
	list_add_tail(&(new->blacklist_member), &blacklist_head);
	return 1;
}

int sys_unblock_program(const char *name, unsigned int name_len)
{
	printk("In Dummy Function: sys_unblock_program %s %d", name, name_len);
	return 2;
}

int sys_get_blocked_count(void)
{
	printk("In Dummy Function: sys_get_blocked_count");
	return 4;
}

int sys_get_forbidden_tries (int pid, char log[][256], unsigned int n)
{
	printk("In Dummy Function: sys_get_forbidden_tries %d %d", pid, n);	// Didnt print log
	return 5;
}
