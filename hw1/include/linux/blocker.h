
// #BENITZIK: All of our code wrappers.

#include <errno.h>

// New struct for list of blocked attempts.
struct blocked_programs_t {
	struct list_head list;
	char* blocked_name[256];
};

struct blocked_programs_t blacklist_programs;

int block_program(const char *name, unsigned int name_len) 
{
	unsigned int res;
	__asm__
	(
		"int $0x80;"
		: "=a" (res)
		: "0" (243) ,"b" (name) ,"c" (name_len)
		: "memory"
	);
	if (res>= (unsigned long)(-125))
	{
		errno = -res;
		res = -1;
	}
	return (int) res;
}

int unblock_program (const char *name, unsigned int
name_len) 
{
	unsigned int res;
	__asm__
	(
		"int $0x80;"
		: "=a" (res)
		: "0" (244) ,"b" (name) ,"c" (name_len)
		: "memory"
	);
	if (res>= (unsigned long)(-125))
	{
		errno = -res;
		res = -1;
	}
	return (int) res;
}

int is_program_blocked(const char *name, unsigned int name_len)
{
	unsigned int res;
	__asm__
	(
		"int $0x80;"
		: "=a" (res)
		: "0" (245) ,"b" (name) ,"c" (name_len)
		: "memory"
	);
	if (res>= (unsigned long)(-125))
	{
		errno = -res;
		res = -1;
	}
	return (int) res;
}

int get_blocked_count (void)
{
	unsigned int res;
	__asm__
	(
		"int $0x80;"
		: "=a" (res)
		: "0" (246)
		: "memory"
	);
	if (res>= (unsigned long)(-125))
	{
		errno = -res;
		res = -1;
	}
	return (int) res;
}

int get_forbidden_tries (int pid, char log[][256], unsigned int n)
{
	unsigned int res;
	__asm__
	(
		"int $0x80;"
		: "=a" (res)
		: "0" (247) ,"b" (pid) ,"c" (log), "d" (n)
		: "memory"
	);
	if (res>= (unsigned long)(-125))
	{
		errno = -res;
		res = -1;
	}
	return (int) res;
}

