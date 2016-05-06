//hw2_syscalls.h


#ifndef _HW2_SYSCALLS_H
#define _HW2_SYSCALLS_H


#include <errno.h>

int is_SHORT(int pid){//syscall #243
	unsigned int res;
	__asm__
	(
		"int $0x80;"
		: "=a" (res)
		: "0" (243) ,"b" (pid)
		: "memory"
	);
	if (res>= (unsigned long)(-125))
	{
		errno = -res;
		res = -1;
	}
	return (int) res;
}

int remaining_time(int pid){//syscall #244
	unsigned int res;
	__asm__
	(
		"int $0x80;"
		: "=a" (res)
		: "0" (244) ,"b" (pid)
		: "memory"
	);
	if (res>= (unsigned long)(-125))
	{
		errno = -res;
		res = -1;
	}
	return (int) res;
}

int remaining_cooloffs(int pid){//syscall #245
	unsigned int res;
	__asm__
	(
		"int $0x80;"
		: "=a" (res)
		: "0" (245) ,"b" (pid)
		: "memory"
	);
	if (res>= (unsigned long)(-125))
	{
		errno = -res;
		res = -1;
	}
	return (int) res;
}




#endif
