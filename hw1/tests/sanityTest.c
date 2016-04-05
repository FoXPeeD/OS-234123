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
	unsigned int name_len = 6;
	int pid = 50;
	unsigned int n = 13;
	char ret_log[n][256];


	assert(block_program(name, name_len)==1);
	assert(unblock_program(name, name_len)==2);
	assert(is_program_blocked(name, name_len)==3);
	assert(get_blocked_count()==4);
	assert(get_forbidden_tries(pid,ret_log,n)==5);


	return 0;
}
