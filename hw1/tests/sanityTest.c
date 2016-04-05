/*
 * basic_test.c
 *
 *  Created on: 5 ????? 2016
 *      Author: user
 */


#include "blocker.h"

int main(){

	const char* name = "hello\0";
	unsigned int name_len = 6;
	int pid = 50;
	unsigned int n = 13;
	char ret_log[n][256];


	block_program(name, name_len);
	unblock_program(name, name_len);
	is_program_blocked(name, name_len);
	get_blocked_count();
	get_forbidden_tries(pid,ret_log,n);




	return 0;
}
