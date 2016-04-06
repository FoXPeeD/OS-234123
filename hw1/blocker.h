
// #BENITZIK: All of our code wrappers. UserSpace!!

#include <errno.h>
#include <stdio.h>

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
	//printf("[res=%d]",res);
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
	printf("[res=%d]",res);
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
	printf("[res=%d]",res);
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
	printf("[res=%d]",res);
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
	printf("[res=%d]",res);
	if (res>= (unsigned long)(-125))
	{
		errno = -res;
		res = -1;
	}
	return (int) res;
}

