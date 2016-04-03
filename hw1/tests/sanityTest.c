#include "blocker.h"
#include <stdlib.h>


int main(){
	const char* name = "hello\0";
	unsigned int name_len = 6;
	int pid = 50;
	unsigned int n = 13;
	char* ret_log = malloc(*size_of(char)*256*n);
	
	block_program(name, name_len);
	unblock_program(name, name_len);
	is_program_blocked(name, name_len);
	get_blocked_count();
	get_forbidden_tries(pid,ret_log,n);

	return 0;
}