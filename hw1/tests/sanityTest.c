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

	const char* name = "hello\0";
	const char* name1 = "/root/dummy1";
	char *argva[] = {"/root/dummy1", 0};
	unsigned int name_len = 6;
	int pid = 50;
	unsigned int n = 13;
	char ret_log[n][256];

	printf("running execve\n");
	execve(name1,argva);
	printf("after execve");
	
	
	printf("\nblock_program");
	block_program(name, name_len);
//	assert(block_program(name, name_len)==1);
	printf("\nunblock_program");
	unblock_program(name, name_len);
//	assert(unblock_program(name, name_len)==2);
	printf("\nis_program_blocked");
	is_program_blocked(name, name_len);
//	assert(is_program_blocked(name, name_len)==3);
	printf("\nget_blocked_count");
	get_blocked_count();
//	assert(get_blocked_count()==4);
	printf("\nget_forbidden_tries");
	get_forbidden_tries(pid,ret_log,n);
//	assert(get_forbidden_tries(pid,ret_log,n)==5);


	return 0;
}
