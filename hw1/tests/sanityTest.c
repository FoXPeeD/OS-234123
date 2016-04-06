/*
 * basic_test.c
 *
 *  Created on: 5 ????? 2016
 *      Author: user
 */


#include "blocker.h"
#include "test_utilities.h"
#include <assert.h>


int main(){

	char *name1 = "/root/hw1/helloworld";
	char *argv[] = { "/root/hw1/helloworld", NULL };
	char *envp[] = { NULL };
	unsigned int name_len = 20;
	int pid = 50;
	unsigned int n = 13;
	char ret_log[n][256];
	int res;
	
	res = execve(name1, argv, envp);
	printf("%d", res);
	
	res = block_program(name1, name_len);
	printf("block_program: %d\n", res);
	//assert(res == 0);
	
	res = is_program_blocked(name1, name_len);
	printf("is_program_blocked: %d\n", res);
	//assert(res == 1);
	
	res = get_blocked_count();
	printf("get_blocked_count: %d\n", res);
	assert(res == 1);
	
	res = execve(name1, argv, envp);
	printf("%d", res);
	
	res = unblock_program(name1, name_len);
	printf("unblock_program: %d\n", res);
	assert(res == 0);

	res = is_program_blocked(name1, name_len);
	printf("is_program_blocked: %d\n", res);
	assert(res == 0);
	
	res = get_blocked_count();
	printf("get_blocked_count: %d\n", res);
	assert(res == 0);

/*
	res = get_forbidden_tries(pid,ret_log,n);
	printf("get_forbidden_tries: %d\n", res);
	assert(res == 1);
*/
	
	
	return 0;
}
