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

	const char* name = "hello";
	const char* name1 = "/root/myTests/dummy1";
	char *argva[] = {"/root/myTests/dummy1", 0};
	unsigned int name_len = 5;
	int pid = 50;
	unsigned int n = 13;
	char ret_log[n][256];
	int res;
	
	//printf("running execve\n");
	//execve(name1,argva);
	//printf("after execve");
	
/*
	res = block_program(name, name_len);
	printf("block_program: %d\n", res);
	//assert(res == 1);
*/	
	res = unblock_program(name, name_len);
	printf("unblock_program: %d\n", res);
	//assert(res == 1);

	
	res = is_program_blocked(name, name_len);
	printf("is_program_blocked: %d\n", res);
	//assert(res == 1);

/*	
	res = get_blocked_count();
	printf("get_blocked_count: %d\n", res);
	assert(res == 1);

	
	res = get_forbidden_tries(pid,ret_log,n);
	printf("get_forbidden_tries: %d\n", res);
	assert(res == 1);
*/
	//WOOOOHOOOOO
	
	
	return 0;
}
